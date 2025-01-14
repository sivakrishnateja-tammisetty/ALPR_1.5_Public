#ifndef TJSON_H
#define TJSON_H

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>

using json = nlohmann::json;

///<  ----------------------------------------------------------------------------------------
class Tjson
{
friend class ThreadCam;
friend class TProcessPipe;
public:
    Tjson();
    ~Tjson();
    bool LoadFromFile(const std::string FileName);
    bool GetSettings(void);
    void MakeFolders(void);
    //  ------------------------------------------------------
    //  your config.json settings
    int UsedStreams;          ///<  number of used streams (usually nr of cameras)
    int WorkWidth;            ///<  Width of the processed frames (small->fast, large->accurate but slow)
    int WorkHeight;           ///<  Height of the processed frames
    int ThumbWidth;           ///<  Width of the frames shown in the overview (8090 output)
    int ThumbHeight;          ///<  Height of the shown frames
    //   filter objects
    int MinPersonSize;        ///<  Minimal size of person to be investigated
    int MinTwoSize;           ///<  Minimal size of 2 wheeler to be investigated
    int MinFourSize;          ///<  Minimal size of 4 wheeler to be investigated
    int MinTwoTicks;          ///<  Minimal valid ticks of 2 wheeler to be investigated
    int MinFourTicks;         ///<  Minimal valid ticks of 4 wheeler to be investigated
    //   additional plate parameters.
    double PlateRatio;        ///<  Minimal Width/Height ratio of the plate (usual around 4 with a one liner)
    int PlateNrChars;         ///<  Number of characters on a plate
    bool FrameOcrOn;          ///<  OCR every license plate in every frame (debug and render purpose)
    bool HeuristicsOn;        ///<  Sort character position and remove doubles
    //   inspection
    double InspectTime;       ///<  Minimal time in Sec person must be close to vehicle (float can be 2.5 Sec)
    double InspectDistance;   ///<  Minimal distance between vehicle and person (ratio of width)
    //   output
    int LogLevel;             ///<  Logger severity (0=off | 6=all)
    bool MQTT_On;             ///<  output json though MQTT
    int JSON_Port;            ///<  output stream json (0 = no stream, usually 8070)
    int MJPEG_Port;           ///<  output stream mjpeg (0 = no stream, usually 8090)
    int MJPEG_Width;          ///<  output image width 8090 stream
    int MJPEG_Height;         ///<  output image height 8090 stream
    bool PrintOnCli;          ///<  Show license plate on teminal
    bool PrintOnRender;       ///<  show license plate in window
    bool ShowBirdView;        ///<  show bird view window
    bool MovingPortal;        ///<  can we adapt the portal position?
    //   administration
    std::string  Version;     ///<  Version string
    //   DNN models
    std::string  Cstr;        ///<  Car darknet model file name
    std::string  Lstr;        ///<  License darknet model file name
    std::string  Ostr;        ///<  OCR darknet model file name
    std::string  Pstr;        ///<  Portal file name
    double ThresCar;          ///<  threshold detection of car model
    double ThresPlate;        ///<  threshold detection of license plate model
    double ThresOCR;          ///<  threshold detection of ocr model
    //   folders
    std::string FoI_Folder;   ///<  directory with analysed frames
    std::string Car_Folder;   ///<  directory with analysed cars
    std::string Plate_Folder; ///<  directory with analysed plates
    std::string Json_Folder;  ///<  directory with json outputs
    std::string Render_Folder;///<  directory with ocr results
    //  ------------------------------------------------------
private:
    json j;
    bool Jvalid;
    bool GetSetting(const json& s, const std::string Key, int& Value);
    bool GetSetting(const json& s, const std::string Key, bool& Value);
    bool GetSetting(const json& s, const std::string Key, double& Value);
    bool GetSetting(const json& s, const std::string Key, std::string& Value);
};
///<  ----------------------------------------------------------------------------------------

#endif ///<   TJSON_H
