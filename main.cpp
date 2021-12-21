#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <bitset>
#include <numeric>


using namespace cv; // OpenCV API is in the C++ "cv" namespace
namespace fs= std::filesystem;


const char* keys =
        "{ help  h| | Print help message. }"
        "{ @input1 | Images/carriers/grayscale_carrier-01.png  | carrier}"
        "{ message |  Images/messages/message1.png |  message  }"
        "{ encode | false  | set to true if decoding an image }"
        "{ password |   | user supplied password }";


std::bitset<8> getBitset(unsigned char testvalue);
void coder(Mat &Message, Mat &encoded, Mat &decoded, bool decode, fs::path inpath);
void write(Mat &outfile,fs::path p,bool decode);
unsigned long djb2_hash(unsigned char *str);
typedef cv::Point2i PixelPos;

void swapPix(Mat_<PixelPos> &ImageKey, Mat &scramMess, int r, int c);

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

void makeKey(std::string &password, int rows, int cols, Mat_<PixelPos> &pPos) {
    unsigned long djb2Hash= djb2_hash((unsigned char*)password.data());
    for(int r=0;r < rows;  r++){
        for(int c=0;c < cols;  c++){
            int newR = RNG(djb2Hash + r*c ).uniform(0, rows);
            int newC = RNG(djb2Hash +c*r).uniform(0, cols);
        pPos.at<PixelPos>(r,c)=pPos.at<PixelPos>(newR,newC);
        }
    }
}

void encrypt(Mat_<PixelPos> &imageKey, Mat &scramMess) {
    for(int r=0; r < imageKey.rows; r++){
        for(int c=0;c < imageKey.cols;  c++){
            swapPix(imageKey, scramMess, r, c);
        }
    }
}

int main(int argc, char** argv){

    CommandLineParser parser(argc, argv, keys);
    fs::path inpath = parser.get<String>("@input1"); //bring in as a fs::path so we deduce a suitable output.
    Mat Carrier = imread(inpath.string(), IMREAD_GRAYSCALE ) ;
    Mat Message=Mat::zeros(Carrier.size(),CV_8UC1);
    Mat encoded, decoded;
    Message.copyTo(decoded);

    // if encrypting

    bool encode = parser.get<bool>("encode");
    Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
    std::string  password = parser.get<std::string >("password");

        int rows = Carrier.rows;
        int cols = Carrier.cols;
        Mat_<PixelPos> pPos = getPposMat(rows, cols);
    if(password.size()!=0){
        //Mat scramMess = Message.clone();
        makeKey(password, rows, cols, pPos);

    }





    // if encoded
        Carrier.copyTo(encoded);
    if(encode == true) {
        //Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
        if(password.size()!=0){
            encrypt(pPos, Message);
        }
        coder(Message, encoded, decoded, false, inpath);
        write(encoded, inpath, false);

        const std::string windowName1 = "Message";
        const std::string windowName2 = "Encoded";
        namedWindow(windowName1, 1);
        namedWindow(windowName2, 1);
        imshow(windowName1, Message);
        imshow(windowName2, encoded);
    }else{
        const std::string decryptMessage = "Message";
        const std::string cryptMessage = "CryptMessage";
        const std::string CarrierWin = "carrier";
        namedWindow(decryptMessage, 1);
        namedWindow(cryptMessage, 1);
        namedWindow(CarrierWin, 1);

        coder(Message, encoded, decoded, true, inpath);
        imshow(cryptMessage, decoded);
        if(password.size()!=0){
            decrypt(pPos, decoded);
        }
        write(decoded, inpath, true);
        imshow(decryptMessage, decoded);
        imshow(CarrierWin, Carrier);
    }










    char c = (char)waitKey(0);

    return  0;

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
