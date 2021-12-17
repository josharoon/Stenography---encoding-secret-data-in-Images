#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <bitset>



using namespace cv; // OpenCV API is in the C++ "cv" namespace
namespace fs= std::filesystem;


const char* keys =
        "{ help  h| | Print help message. }"
        "{ @input1 | Images/carriers/grayscale_carrier-01.png  | carrier}"
        "{ @input2 |  Images/messages/message1.png |  message  }"
        "{ @input3 | output16000.jpeg  | Path to input image 3. }";


std::bitset<8> getBitset(unsigned char testvalue);

unsigned char getBitN(unsigned char val, int n) {

    unsigned char b;
    b = 1 & (val >> n);
    return b;
}

int main(int argc, char** argv){

    CommandLineParser parser(argc, argv, keys);
    std::cout << "filename" <<  parser.get<String>("@input1");
    Mat Carrier = imread(parser.get<String>("@input1"),  IMREAD_GRAYSCALE ) ;
    Mat Message =  imread(parser.get<String>("@input2"),  IMREAD_GRAYSCALE ) ;
    bitwise_not(Message, Message);
    Mat encoded, decoded;
    Carrier.copyTo(encoded);
    Carrier.copyTo(decoded);


    const std::string windowName = "carrier";
    const std::string windowName2 = "Message";
    const std::string windowName3 = "Encoded";
    const std::string windowName4 = "Decoded";

    // create window object
    namedWindow(windowName, 1);
    namedWindow(windowName2, 1);

//    namedWindow(windowName4, 1);

    imshow(windowName, Carrier);
    imshow(windowName2, Message);


    typedef cv::Point_<uint8_t> Pixel;

    encoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {
        std::bitset<8> bpix = getBitset(pixel);
        int messagePix=Message.at<char>(position[0],position[1]);
        //std::cout<<messagePix;
        //bpix.reset();
        bpix[0] =  abs(messagePix);

        int newPix=(int)(bpix.to_ulong());
        encoded.at<char>(position[0],position[1])=newPix;


    });


    decoded.forEach<uint8_t>([&](uint8_t& pixel, const int position[]) -> void {

        std::bitset<8> bpix = getBitset(encoded.at<char>(position[0],position[1]));
        int messagePix=bpix[0];
        //std::cout << bpix[0] << std::endl;
        int newPix=(int)(bpix.to_ulong());
        decoded.at<char>(position[0],position[1])=bpix[0]*255;



    });
    namedWindow(windowName3, 1);
    imshow(windowName3, encoded);
    namedWindow(windowName4, 1);
    imshow(windowName4, decoded);




    unsigned char testvalue = 5;
    std::bitset<8> pixel = getBitset(testvalue);
    // useful test values: 10101010 = 170, 01010101 = 85, 11111111 = 255
    // 11111111 = 255, 11110000 = 240, 00001111 = 15



    std::cout << "decimal value %i in binary is = " << pixel.to_string();


    char c = (char)waitKey(0);

    return  0;

}

std::bitset<8> getBitset(unsigned char testvalue) {
    std::bitset<8> pixel(testvalue);
    return pixel;
}
