#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <bitset>
#include <numeric>
#include <fstream>
#include <string>


using namespace cv; // OpenCV API is in the C++ "cv" namespace
namespace fs= std::filesystem;


const char* keys =
        "{ help  h| | Print help message. }"
        "{ @input1 | Images/carriers/grayscale_carrier-01.png  | carrier}"
        "{ message |   | image  message  }"
        "{ txt |   |  input text File  }"
        "{ outtxt |   |  output text File  }"
        "{ encode | false  | set to true if encoding an image }"
        "{ decode | false  | set to true if decoding an image }"
        "{ password |   | user supplied password }"
        "{ RGB | false  | read 3 channel RGB Image }"
"{ noise | false  | add Gaussian Noise }";


std::bitset<8> getBitset(unsigned char testvalue);
void coder(Mat &Message, Mat &encoded, Mat &decoded, bool decode, fs::path inpath);
void write(Mat &outfile,fs::path p,bool decode);
unsigned long djb2_hash(unsigned char *str);
typedef cv::Point2i PixelPos;

void swapPix(Mat_<PixelPos> &ImageKey, Mat &scramMess, int r, int c);

void txt2Message(Mat &MessageMat, String &MessageStr);
void Message2Txt(Mat &MessageMat, String &MessageStr);
void Message2File(String &Message, fs::path p);

void decrypt(Mat_<PixelPos> &keyMat, Mat &encodImage) {
    for(int r= keyMat.rows - 1; r >= 0; r--){
        for(int c= keyMat.cols - 1; c >= 0; c--){
            swapPix(keyMat, encodImage, r, c);
        }
    }
}

Mat_<PixelPos> getPposMat(int rows, int cols) {
    Mat_<PixelPos> pPos = Mat::zeros(rows, cols, CV_8UC2);
    pPos.forEach([&](PixelPos& pixel, const int position[]) -> void {
        pixel.x = position[0];
        pixel.y = position[1];
    });
    return pPos;
}

void makeKey(std::string &password, int rows, int cols, Mat_<PixelPos> &pPos, unsigned long hash) {

    for(int r=0;r < rows;  r++){
        for(int c=0;c < cols;  c++){
            int newR = RNG(hash + r*c ).uniform(0, rows);
            int newC = RNG(hash +c*r).uniform(0, cols);
        pPos.at<PixelPos>(r,c)=pPos.at<PixelPos>(newR,newC);
        }
    }
}

void encodeRGB(std::string &password, int rows, int cols, Mat &Message, unsigned long hash, Mat &rgbImage) {
    bitwise_not(Message, Message);
    Mat_<Vec3b> m=Mat::zeros({cols,rows},CV_8UC3);
    RNG rng=RNG(hash);
    for(int r=0;r < rows;  r++){
        for(int c=0;c < cols;  c++){
            //use  the  password  seeded  random  number  generator  to  select  a  random  location  (c,r)

            int newR = rng.uniform(0, rows);
            int newC = rng.uniform(0, cols);
            int channel=rng.uniform(0, 3);



            while(m.at<Vec3b>(newR,newC)[channel]==1){

                newR = rng.uniform(0, rows);
                newC = rng.uniform(0, cols);
                channel=rng.uniform(0, 3);


            }

                int pix1=rgbImage.at<Vec3b>(newR,newC)[channel];
                std::bitset<8> bpix = getBitset(pix1);
                int messagePix=Message.at<unsigned char>(r,c);
                bpix[0] =getBitset(messagePix)[0];
                int newPix=(int)(bpix.to_ulong());
                rgbImage.at<Vec3b>(newR,newC)[channel]=newPix;
                (m.at<Vec3b>(newR,newC)[channel]) =1;


        }
    }

}

void decodeRGB(std::string &password, int rows, int cols, Mat &Message, unsigned long hash, Mat &rgbImage) {

    RNG rng = RNG(hash);
    Mat_<Vec3b> m=Mat::zeros({cols,rows},CV_8UC3);
            for(int r=0;r < rows;  r++){
                for(int c=0;c < cols;  c++){


                    int newR = rng.uniform(0, rows);
                    int newC = rng.uniform(0, cols);
                    int channel=rng.uniform(0, 3);



                    while(m.at<Vec3b>(newR,newC)[channel]==1){

                        newR = rng.uniform(0, rows);
                        newC = rng.uniform(0, cols);
                        channel=rng.uniform(0, 3);


                        }
                        //std::cout << "swapping" << "," << newR << "," << newC << "," <<  channel  << "," << " with " << "," << r << "," << c << std::endl;
                        int pix1 = rgbImage.at<Vec3b>(newR, newC)[channel];
                        std::bitset<8> bpix = getBitset(pix1);
                        int messagePix = bpix[0];
                        //int newPix = (int) (messagePix.to_ulong());
                        Message.at<uchar>(r, c) = messagePix*255;
                        m.at<Vec3b>(newR,newC)[channel] =1;

            }
        }
    bitwise_not(Message, Message);
    }

//get string from text file.
String getTxtFile(fs::path filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        return(String((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    }
    throw(errno);
}



void encrypt(Mat_<PixelPos> &imageKey, Mat &scramMess) {
    for(int r=0; r < imageKey.rows; r++){
        for(int c=0;c < imageKey.cols;  c++){
            swapPix(imageKey, scramMess, r, c);
        }
    }
}

int main(int argc, char** argv){

    // Parse Command Line and Load Images
    CommandLineParser parser(argc, argv, keys);
    fs::path inpath = parser.get<String>("@input1"); //bring in as a fs::path so we deduce a suitable output.
    fs::path textPath=parser.get<String>("txt");
    fs::path outTxt=parser.get<String>("outtxt");

    bool encode = parser.get<bool>("encode");
    bool decode = parser.get<bool>("decode");
    bool noise = parser.get<bool>("noise");
    bool rgb = parser.get<bool>("RGB");

    //setup rand generator
    std::string  password = parser.get<std::string >("password");
    unsigned long hash = djb2_hash((unsigned char *) password.data());
    RNG  rGen=RNG(hash);
    String txtMessage="";
    //Read carrier as greyscale or Color.
    ImreadModes flags = IMREAD_GRAYSCALE;
    //handle RGB case
    if(rgb){flags=cv::IMREAD_COLOR;




    }
    Mat Carrier = imread(inpath.string(), flags) ;



    //read message Image

    Mat Message=Mat::zeros(Carrier.size(),CV_8UC1);
    Mat Message2=Message.clone();
    if(parser.get<String>("message")!="") Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
    if(parser.get<String>("txt")!="") {  //convert text message into Mat for RGB encoding.
        txtMessage = getTxtFile(textPath);
        txt2Message(Message, txtMessage);
    }

    //read message Text file


    // convert string to binary.
    // add string to message matrix.





    //setup generic windows
    std::string windowName1;
    std::string windowName2;
    std::string windowName3;




    //set up Necessary Matrices.
    Mat encoded, decoded;
    Message.copyTo(decoded);
        int rows = Carrier.rows;
        int cols = Carrier.cols;
        Mat_<PixelPos> pPos = getPposMat(rows, cols);
    if(password.size()!=0){

        makeKey(password, rows, cols, pPos, hash);

    }

    windowName1  = "message";
    namedWindow(windowName1, 1);
    imshow(windowName1, Message);



    //encode decode into RGB images.
    if(encode && rgb ) {
        encodeRGB(password, rows, cols, Message, hash, Carrier);
        write(Carrier, inpath,false);
    }

    windowName3  = "carrier";
    namedWindow(windowName3, 1);
    imshow(windowName3, Carrier);

    if(decode && rgb )decodeRGB(password, rows, cols, Message2, hash, Carrier);


    windowName2  = "message decoded";
    namedWindow(windowName2, 1);
    imshow(windowName2, Message2);

    if(outTxt !="") {  //convert text message into Mat for RGB encoding.
        Message2Txt(Message2, txtMessage);
        Message2File(txtMessage, outTxt);
    }




    if(noise) {
        windowName2 = "Carrier";
        namedWindow(windowName2, 1);
        imshow(windowName2, Carrier);
        Carrier.forEach<uint8_t>([&](uint8_t &pixel, const int position[]) -> void {
           pixel+=rGen.gaussian(0.5);
        });
            windowName3 = "Noise";
            namedWindow(windowName3, 1);
            imshow(windowName3, Carrier);
    }
    // execute Image processing, display and write results.
        Carrier.copyTo(encoded);
    if(encode == true && !rgb) {
        //Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
        if(password.size()!=0){
            encrypt(pPos, Message);
        }
        coder(Message, encoded, decoded, false, inpath);
        write(encoded, inpath, false);

        windowName1 = "Message";
        windowName2 = "Encoded";
        namedWindow(windowName1, 1);
        namedWindow(windowName2, 1);
        imshow(windowName1, Message);
        imshow(windowName2, encoded);
    }else if(decode == true && !rgb){
        windowName1 = "Message";
        windowName2 = "CryptMessage";
        windowName3  = "carrier";
        namedWindow(windowName1, 1);
        namedWindow(windowName2, 1);
        namedWindow(windowName3, 1);

        coder(Message, encoded, decoded, true, inpath);
        imshow(windowName2, decoded);
        if(password.size()!=0){
            decrypt(pPos, decoded);
        }
        write(decoded, inpath, true);
        imshow(windowName1, decoded);
        imshow(windowName3, Carrier);
    }
    char c = (char)waitKey(0);
    return  0;

}

void txt2Message(Mat &MessageMat, String &MessageStr) {
    int bytes=MessageStr.length();
    //calculate capacity of  RGB image of same size as MessageMat in bytes
    int MessCapacity=(MessageMat.rows*MessageMat.cols*3)/8;
    // reserve first 8 bits to encode message length up to 255 bytes (0 represents 1 byte).
    std::bitset<8> bytesAsbits=getBitset(bytes);
    for(int bit=0 ; bit < 8 ; bit ++){
        int col = bit % MessageMat.rows;
        int row = bit / MessageMat.rows;
        int MessageVal = bytesAsbits[bit];
        MessageMat.at<uint8_t>(row, col)= MessageVal;
    }
    //encode string into mat.
    int bitCount=bytesAsbits.size();
    for (int i=0;i <MessageStr.length();i++){
        //get char as bitset
        std::bitset<8>  c(MessageStr[i]);
        for(int bit=bitCount ; bit < bitCount+8 ; bit ++) {
            int col = bit % MessageMat.rows;
            int row = bit / MessageMat.rows;
            int MessageVal = c[bit - bitCount];
            MessageMat.at<uint8_t>(row, col)= MessageVal;
        }
        bitCount+=8;
    }
}


void Message2Txt(Mat &MessageMat, String &MessageStr){
    //get length of message in bytes
    std::bitset<8> bytes;
    for(int bit=0 ; bit < 8 ; bit ++) {
        int col = bit % MessageMat.rows;
        int row = bit / MessageMat.rows;
        bytes[bit] = MessageMat.at<uint8_t>(row, col);
    }
    //loop through Message matrix converting each 8 bits into characters and adding them to message string.
    int MessLength = (int) bytes.to_ulong();
    std::bitset<8> character;
    int bitCount=8; //assume message length is stored in 8 bits.
    for(int i=0; i < MessLength; i++){
        for(int bit=bitCount ; bit < bitCount+8 ; bit ++) {

            int col = bit % MessageMat.rows;
            int row = bit / MessageMat.rows;
            character[bit-bitCount] = MessageMat.at<uint8_t>(row, col);
        }
        bitCount+=8;
        char c=(int)character.to_ulong();
        MessageStr.push_back(c);
        
        

    }


}


void swapPix(Mat_<PixelPos> &ImageKey, Mat &scramMess, int r, int c) {
    cv::Point2i newPos=ImageKey.at<cv::Point2i>(r, c);
    cv::Point2i oldPos= cv::Point2i(r, c);

    int pix=scramMess.at<uchar>(newPos.x,newPos.y);
    int pix1=scramMess.at<uchar>(r,c);
    //std::cout << "swapping" << newPos << " with " << oldPos << std::endl;

    scramMess.at<uchar>(newPos.x, newPos.y) = pix1;
    scramMess.at<uchar>(oldPos.x,oldPos.y) = pix;

}

void coder(Mat &Message, Mat &encoded, Mat &decoded, bool decode, fs::path inpath) {
    typedef Point_<uint8_t> Pixel;

    if(decode) {  decoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
            std::bitset<8> bpix = getBitset(encoded.at<char>(position[0],position[1]));
            decoded.at<char>(position[0],position[1])=bpix[0]*255;

        });
        bitwise_not(decoded, decoded); // invert image back;
        //write(decoded, inpath, true);
    }else{

        bitwise_not(Message, Message); // invert image so less pixels with val1;
        encoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
            std::bitset<8> bpix = getBitset(pixel);
            int messagePix=Message.at<char>(position[0],position[1]);
            bpix[0] =  abs(messagePix);
            int newPix=(int)(bpix.to_ulong());
            encoded.at<char>(position[0],position[1])=newPix;});

        //write(encoded, inpath, false);
        }

}

std::bitset<8> getBitset(unsigned char testvalue) {
    std::bitset<8> pixel(testvalue);
    return pixel;
}

void write(Mat &outfile, fs::path p, bool decode) {

    std::string suffix="_encoded";
    if(decode)suffix="_decoded";

    fs::path outpath=p.stem()+=suffix;
    outpath+=p.extension();
    std::cout << "writing" << outpath.string() << std::endl;
    imwrite(outpath.string(),outfile);

}

//  http://www.cse.yorku.ca/~oz/hash.html   the function djb2_hash code has been taken from here.
unsigned long djb2_hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;


}

void Message2File(String &Message, fs::path p= "test.txt") {


    std::ofstream out(p);
    out << Message;
    out.close();





}
