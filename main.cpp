#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <filesystem>
#include <bitset>
#include <numeric>
#include <fstream>
#include <string>

// Joshua Sutcliffe Image Processing Assignment 2021 Sn 346658

//C++ Standard 17.   This must be compiled with C++17. It has been tested on a windows lab pc with X64 target.

using namespace cv;
namespace fs= std::filesystem;
//define inputs for command line interface.
const char* keys =
        "{ help  h| | Print help message. }"
        "{ @input1 | Images/carriers/grayscale_carrier-01.png  | carrier}"
        "{ diff |   | takes difference between file and carrier}"
        "{ message |   | image  message  }"
        "{ bin |   |  input generic binary File  }"
        "{ outbin |   |  output generic binary File  }"
        "{ encode | false  | set to true if encoding an image }"
        "{ decode | false  | set to true if decoding an image }"
        "{ password |   | user supplied password }"
        "{ rgb | false  | read 3 channel RGB Image }"
"{ noise | false  | add Gaussian Noise }";

//declare functions and types
typedef cv::Point2i PixelPos;

void coder(Mat &Message, Mat &encoded, Mat &decoded, bool decode, const fs::path& inpath);
void write(Mat &outfile, const fs::path& p);
unsigned long djb2_hash(unsigned char *str);
void swapPix(Mat_<PixelPos> &ImageKey, Mat &scramMess, int r, int c);
void Message2Bin(Mat &MessageMat,  std::vector<char> &MessageVec);
void bin2Message(Mat &MessageMat, std::vector<char> &MessageVec);
void bin2File(std::vector<char> &Message, const fs::path& p);
void decrypt(Mat_<PixelPos> &keyMat, Mat &encodImage) {
    for(int r= keyMat.rows - 1; r >= 0; r--){
        for(int c= keyMat.cols - 1; c >= 0; c--){
            swapPix(keyMat, encodImage, r, c);
        }
    }
}
Mat_<PixelPos> getPposMat(int rows, int cols);
void makeKey(std::string &password, int rows, int cols, Mat_<PixelPos> &pPos, unsigned long hash);
void encodeRGB(std::string &password, int rows, int cols, Mat &Message, unsigned long hash, Mat &rgbImage);
void decodeRGB(std::string &password, int rows, int cols, Mat &Message, unsigned long hash, Mat &rgbImage);
std::vector<char> getBinFile(const fs::path& filename);
void encrypt(Mat_<PixelPos> &imageKey, Mat &scramMess);
//main branch   accepts arguments from command line.
int main(int argc, char** argv){

    // Parse Command Line and Load Images
    CommandLineParser parser(argc, argv, keys);
    fs::path inpath = parser.get<String>("@input1"); //bring in as a fs::path so we deduce a suitable output.
    fs::path binPath=parser.get<String>("bin");
    fs::path outBin=parser.get<String>("outbin");
    fs::path diffPath = parser.get<String>("diff");
    bool encode = parser.get<bool>("encode");
    bool decode = parser.get<bool>("decode");
    bool noise = parser.get<bool>("noise");
    bool rgb = parser.get<bool>("rgb");
    auto  password = parser.get<std::string >("password");
    String txtMessage;
    std::vector<char> binMessage={};

    //setup rand generator
    unsigned long hash = djb2_hash((unsigned char *) password.data());
    RNG  rGen=RNG(hash);
    //Read carrier as greyscale or Color.
    ImreadModes flags = IMREAD_GRAYSCALE;
    if(rgb)flags=cv::IMREAD_COLOR;
    Mat Carrier = imread(inpath.string(), flags) ;
    //Get dofference between 2 images
    if(diffPath !=""){
        Mat Diff = imread(diffPath.string(), flags) ;
        absdiff(Carrier,Diff,Diff);
        write(Diff,fs::path((inpath.stem()+=diffPath.stem()  += "_diff") += inpath.extension()));
    }

    //read message Image if encoding
    Mat Message=Mat::zeros(Carrier.size(),CV_8UC1);
    Mat Message2=Message.clone();
    //setup generic windows
    std::string windowName1;
    std::string windowName2;
    std::string windowName3;

    //set up Necessary Matrices.
    Mat encoded, decoded;
    Message.copyTo(decoded);
        int rows = Carrier.rows;
        int cols = Carrier.cols;

    //make look up matrix for greyscale encoding
    Mat_<PixelPos> pPos = getPposMat(rows, cols);

    //Handle different command line options.

    if(!parser.get<String>("message").empty()) Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
    if(!parser.get<String>("bin").empty()) {  //convert text message into Mat for RGB encoding.
        std::vector<char> messageVec = getBinFile(binPath);
        bin2Message(Message, messageVec);
    }
    if(!password.empty()){
        makeKey(password, rows, cols, pPos, hash);
    }

    //encode decode into RGB images.
    if(encode && rgb ) {
        encodeRGB(password, rows, cols, Message, hash, Carrier);
        write(Carrier, fs::path((inpath.stem() += "_encoded") += inpath.extension()));
    }

    if(decode && rgb ) {
        decodeRGB(password, rows, cols, Message2, hash, Carrier);
        // if working with text files output to text file.

            if (outBin !="") {
                Message2Bin(Message2, binMessage);
                bin2File(binMessage, outBin);
            } else {
                write(Message2, fs::path((inpath.stem() += "_decoded") += inpath.extension()));
            }

    }
    // add noise to carrier Image
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
        write(Carrier, fs::path((inpath.stem() += "_noise") += inpath.extension()));

    }
    // execute Image processing, display and write results.
        Carrier.copyTo(encoded);
    //handle greyscale encoding and decoding
    if(encode  && !rgb) {
        //Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
        if(!password.empty()){
            encrypt(pPos, Message);
        }
        coder(Message, encoded, decoded, false, inpath);
        write(encoded, fs::path((inpath.stem() += "_encoded") += inpath.extension()));
        bitwise_not(Message,Message);
        windowName1 = "Message";
        windowName2 = "Encoded";
        namedWindow(windowName1, 1);
        namedWindow(windowName2, 1);
        imshow(windowName1, Message);
        imshow(windowName2, encoded);
    }

    if(decode  && !rgb){

        coder(Message, encoded, decoded, true, inpath);
        if(!password.empty()){
            decrypt(pPos, decoded);
        }
        windowName1 = "DeCryptMessage";
        write(decoded, fs::path((inpath.stem() += "_decoded") += inpath.extension()));
        imshow(windowName1, decoded);
    }

    std::cout << "Press any key to Finish";
    char c = (char)waitKey(0);
    return  0;

}

// implement functions

// Swap pixels using an image key generated from a password
void swapPix(Mat_<PixelPos> &ImageKey, Mat &scramMess, int r, int c) {
    cv::Point2i newPos=ImageKey.at<cv::Point2i>(r, c);
    cv::Point2i oldPos= cv::Point2i(r, c);
    int pix=scramMess.at<uchar>(newPos.x,newPos.y);
    int pix1=scramMess.at<uchar>(r,c);
    scramMess.at<uchar>(newPos.x, newPos.y) = pix1;
    scramMess.at<uchar>(oldPos.x,oldPos.y) = pix;
}
// Encoder/Decoder for Grayscale Images
void coder(Mat &Message, Mat &encoded, Mat &decoded, bool decode, const fs::path& inpath) {
    typedef Point_<uint8_t> Pixel;
    if(decode) {  decoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
            unsigned char testvalue = encoded.at<char>(position[0], position[1]);
            std::bitset<8> pixel1(testvalue);
            std::bitset<8> bpix = pixel1;
            decoded.at<char>(position[0],position[1])=bpix[0]*255;

        });
        bitwise_not(decoded, decoded); // invert image back;
    }else{
        bitwise_not(Message, Message); // invert image so less pixels with 1 value;
        encoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
            std::bitset<8> pixel1(pixel);
            std::bitset<8> bpix = pixel1;
            int messagePix=Message.at<char>(position[0],position[1]);
            bpix[0] =  abs(messagePix);
            int newPix=(int)(bpix.to_ulong());
            encoded.at<char>(position[0],position[1])=newPix;});
        }
}
//Write out Matrix as Image given a path
void write(Mat &outfile, const fs::path& p) {
    std::cout << "writing" << p.string() << std::endl;
    imwrite(p.string(),outfile);
}
//Our hashing function for encryption
//  http://www.cse.yorku.ca/~oz/hash.html   the function djb2_hash code has been taken from here.
unsigned long djb2_hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
//encode our arbitary binary information into our Carrier Matrix
void bin2Message(Mat &MessageMat, std::vector<char> &MessageVec) {
    int bytes=MessageVec.size();
    //calculate capacity of  RGB image of same size as MessageMat in bytes
    int MessCapacity=(MessageMat.rows*MessageMat.cols*3)/8;
    // Handle case where Message is too big to encode in Image.
    const int headSize = 24;
    if(bytes >= MessCapacity - headSize){
        std::cout << "message to big for Image.  Max size (bytes) allowed =" <<  MessCapacity-headSize;
        return;
    }
    //encode message size as header of 1d/array
    std::vector<int> bits; //1d array of all bits to encode
    std::bitset<headSize> bytesAsbits(bytes);
    bits.reserve(headSize);
for(int i=0; i<headSize;i++)bits.push_back(bytesAsbits[i]);
    //encode rest of message into 1 d array.
    for(char i : MessageVec){
        std::bitset<8> c(i);
        for(int bit=0;bit <8; bit++)bits.push_back(c[bit]);
    }
    for(int i=0;i<MessageMat.rows;i++)
        for(int j=0;j<MessageMat.cols;j++) {
            auto i1 = i * MessageMat.cols + j;
            if(i1==bits.size()){
                return;
            }
            MessageMat.at<uint8_t>(i, j)=bits[i1];
        }
}
//decode our arbitary binary information from our carrier Matrix and back into a message vector
void Message2Bin(Mat &MessageMat, std::vector<char> &MessageVec) {
    int MessLength =0;
    const int headSize = 24;
    std::bitset<headSize> bytes;
    std::vector<int> bits; //1d array to place bit message in.
    std::bitset<headSize> messSize;
    int bitcount=0;
    int endbit=0;
    //loop through message to extract binary information
    for(int i = 0; i < MessageMat.rows; i++)
        for(int j = 0; j< MessageMat.cols; j++)

        {
            if(bitcount<headSize) {
                bytes[bitcount] = MessageMat.at<uint8_t>(i, j);
            }
            if(bitcount==headSize) { //extract header from matrix.
                MessLength = (int) bytes.to_ulong();//get message length as bytes.
                endbit=(MessLength*8+headSize);
            }
            bits.push_back( MessageMat.at<uint8_t>(i, j));
            bitcount++;
            if(bitcount==endbit){
                goto encodeVector;
            }
        }
    encodeVector:
    std::bitset<8> character;
    int bitStart=0;
    //run through are bit vector converting it to char and adding to message vector.
    for (int i=headSize-1; i < bits.size(); i +=8) {
        for (int j = 0; j < 8; j++) {
            character[j] = bits[bitStart];
            bitStart++;
        }
        MessageVec.push_back((int) character.to_ullong());
    }
}
//re write our message vector back into a binary file   .txt and .pdf files have been tested.
void bin2File(std::vector<char> &Message, const fs::path& p) {
    //use of the binary flag to avoid formatting differences using text files.
    std::ofstream out(p, std::ios::binary);
    for (char c: Message)out << c;
    out.close();
}
//Create a matrix of point Positions to be used in creating our key.
Mat_<PixelPos> getPposMat(int rows, int cols) {
    Mat_<PixelPos> pPos = Mat::zeros(rows, cols, CV_8UC2);
    pPos.forEach([&](PixelPos& pixel, const int position[]) -> void {
        pixel.x = position[0];
        pixel.y = position[1];
    });
    return pPos;
}
//encode binary information from our message matrix into  our 3 Channel image Matrix
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
            std::bitset<8> pixel(pix1);
            std::bitset<8> bpix = pixel;
            int messagePix=Message.at<unsigned char>(r,c);
            std::bitset<8> pixel1(messagePix);
            bpix[0] = pixel1[0];
            int newPix=(int)(bpix.to_ulong());
            rgbImage.at<Vec3b>(newR,newC)[channel]=newPix;
            (m.at<Vec3b>(newR,newC)[channel]) =1;
        }
    }
}
//decode binary information from our carrier  image matrix into a Message Matrix
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
            int pix1 = rgbImage.at<Vec3b>(newR, newC)[channel];
            std::bitset<8> pixel(pix1);
            std::bitset<8> bpix = pixel;
            int messagePix = bpix[0];
            //int newPix = (int) (messagePix.to_ulong());
            Message.at<uchar>(r, c) = messagePix*255;
            m.at<Vec3b>(newR,newC)[channel] =1;
        }
    }
    bitwise_not(Message, Message);
}
//Extract binary file into a vector of Char
std::vector<char> getBinFile(const fs::path &filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    return {(std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()};
}
// produce encryption matrix
void encrypt(Mat_<PixelPos> &imageKey, Mat &scramMess) {
    for(int r=0; r < imageKey.rows; r++){
        for(int c=0;c < imageKey.cols;  c++){
            swapPix(imageKey, scramMess, r, c);
        }
    }
}
//make encryption key from password
void makeKey(std::string &password, int rows, int cols, Mat_<PixelPos> &pPos, unsigned long hash) {
    for(int r=0;r < rows;  r++){
        for(int c=0;c < cols;  c++){
            int newR = RNG(hash + r*c ).uniform(0, rows);
            int newC = RNG(hash +c*r).uniform(0, cols);
            pPos.at<PixelPos>(r,c)=pPos.at<PixelPos>(newR,newC);
        }
    }
}


