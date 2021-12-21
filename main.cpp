#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <bitset>
#include <list>
#include <numeric>
#include <algorithm>    // std::random_shuffle
#include <vector>

using namespace cv; // OpenCV API is in the C++ "cv" namespace
namespace fs= std::filesystem;


const char* keys =
        "{ help  h| | Print help message. }"
        "{ @input1 | Images/carriers/grayscale_carrier-01.png  | carrier}"
        "{ message |  Images/messages/message1.png |  message  }"
        "{ decode | false  | set to true if decoding an image }"
        "{ password | password  | user supplied password }";


std::bitset<8> getBitset(unsigned char testvalue);
void coder(Mat &Message, Mat &encoded, Mat &decoded, bool decode, fs::path inpath);
void write(Mat &outfile,fs::path p,bool decode);
unsigned long djb2_hash(unsigned char *str);
typedef cv::Point2i PixelPos;

void swapPix(Mat_<PixelPos> &pPos, Mat &scramMess, int r, int c);

int main(int argc, char** argv){

    CommandLineParser parser(argc, argv, keys);
    fs::path inpath = parser.get<String>("@input1"); //bring in as a fs::path so we deduce a suitable output.


    Mat Carrier = imread(inpath.string(), IMREAD_GRAYSCALE ) ;
    Mat Message=Mat::zeros(Carrier.size(),CV_8UC1);
    Mat encoded, decoded;
    Message.copyTo(decoded);

    // if encrypting
    Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
    std::string  password = parser.get<std::string >("password");
    //unsigned char *str =password.data();
    unsigned long djb2Hash= djb2_hash((unsigned char*)password.data());




    RNG randG(djb2Hash);

    //std::shuffle(indicies.begin(), indicies.end(), randG);
    //std::shuffle(indicies.begin(), indicies.end(), randG);



    Mat_<PixelPos> pPos = Mat::zeros(480,640, CV_8UC2);
//    pPos.forEach([&](PixelPos& pixel, const int position[]) -> void {
//        pixel.x = position[0];
//        pixel.y = position[1];
//    });


    //create matrix of point positions.
    for(int r=0;r < pPos.rows;  r++){

        for(int c=0;c < pPos.cols;  c++){

            //std::cout << r <<","<<c <<":";


            pPos.at<PixelPos>(r,c)=PixelPos(r,c);





        }

    }
    std::cout << pPos.at<PixelPos>(0,0) <<"," << pPos.at<PixelPos>(479,639)<< std::endl;;





    //Update Matrix with positions based on Random Generator.
//    pPos.forEach([&](PixelPos& pixel, const int position[]) -> void {
//        int width = 640 ;//Carrier.size().width;
//        int x = RNG(djb2Hash + position[0]).uniform(0, width);
//        pixel.x = x;
//        int heightMax = 480;
//        int y = RNG(djb2Hash + position[1]).uniform(0, heightMax);
//        pixel.y = y;
//
//
//
//
//
//            //if(pixel.y>478)std::cout << "\n" << pixel << std::endl;
//    });
   // randShuffle(pPos, 1,&randG);


            int width = 640 ;//Carrier.size().width;
            int heightMax = 480;
    Mat scramMess = Message.clone();

    std::cout << pPos.rows << pPos.cols << std::endl;;

    // Create Encode Matrix
    for(int r=0;r < pPos.rows;  r++){
        for(int c=0;c < pPos.cols;  c++){
            int newR = RNG(djb2Hash + r*c ).uniform(0, heightMax);
            int newC = RNG(djb2Hash +c*r).uniform(0, width);
        //std::cout << "swapping" << newR << ","<< newC << " with " << r << "," << c << std::endl;
        pPos.at<PixelPos>(r,c)=pPos.at<PixelPos>(newR,newC);


        randG.next();

        }

    }

    for(int r=0;r < pPos.rows;  r++){
        for(int c=0;c < pPos.cols;  c++){

            swapPix(pPos, scramMess, r, c);


        }

    }

    std::cout << pPos.at<PixelPos>(0,0) <<"," << pPos.at<PixelPos>(479,639) << std::endl;

    Mat decMess = scramMess.clone();
    for(int r=pPos.rows-1; r>= 0;  r--){
        for(int c=pPos.cols-1;c>= 0;  c--){

            swapPix(pPos, decMess, r, c);



        }

    }







//    Message.forEach<uint8_t>([&](uint8_t& pixel, const int *position) -> void {
//          PixelPos newPos;
//          Point currPos = Point(position[10],position[1]);
//
//
//        if (currPos.y < 480 ) {
//          newPos = pPos.at<PixelPos>(currPos);
//            scramMess.at<uchar>(newPos) = Message.at<uchar>(currPos);
//        }
//
//
//
//    });


    const std::string windowName5 = "Scrambled";

    namedWindow(windowName5, 1);
    imshow(windowName5, scramMess);

    const std::string windowName3 = "DesScrambled";

    namedWindow(windowName3, 1);
    imshow(windowName3, decMess);




    // if encoded
        Carrier.copyTo(encoded);
    bool decode = parser.get<bool>("decode");
    if(!decode) {
        //Message =  imread(parser.get<String>("message"),  IMREAD_GRAYSCALE ) ;
        coder(Message, encoded, decoded, false, inpath);
        const std::string windowName1 = "Message";
        const std::string windowName2 = "Encoded";
        namedWindow(windowName1, 1);
        namedWindow(windowName2, 1);
        imshow(windowName1, Message);
        imshow(windowName2, encoded);
    }else{

        coder(Message, encoded, decoded, true, inpath);
        const std::string windowName1 = "Message";
        const std::string windowName2 = "carrier";
        namedWindow(windowName1, 1);
        namedWindow(windowName2, 1);
        imshow(windowName1, decoded);
        imshow(windowName2, Carrier);
    }










    char c = (char)waitKey(0);

    return  0;

}

void swapPix(Mat_<PixelPos> &pPos, Mat &scramMess, int r, int c) {
    cv::Point2i newPos=pPos.at<cv::Point2i>(r, c);
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
        write(decoded, inpath, true);
    }else{

        bitwise_not(Message, Message); // invert image so less pixels with val1;
        encoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
            std::bitset<8> bpix = getBitset(pixel);
            int messagePix=Message.at<char>(position[0],position[1]);
            bpix[0] =  abs(messagePix);
            int newPix=(int)(bpix.to_ulong());
            encoded.at<char>(position[0],position[1])=newPix;});

        write(encoded, inpath, false);
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
