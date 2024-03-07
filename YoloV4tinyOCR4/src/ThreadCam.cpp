#include "ThreadCam.h"

//----------------------------------------------------------------------------------
ThreadCam::ThreadCam(int _Width, int _Height): Tag(0), PrevTag(0), GrabOpen(false), GrabStop(false), GrabTag(0)
{
    Width   = _Width;
    Height  = _Height;
    Device  = "";
    CamName = "";
}
//----------------------------------------------------------------------------------
ThreadCam::~ThreadCam(void)
{
    GrabOpen.store(false);
    cap.release();
}
//----------------------------------------------------------------------------------
void ThreadCam::Start(const json& s)
{
    Tjson TJ;

    //force the json to be valid, else no GetSetting() will work
    TJ.Jvalid=true;

    //reset struct
    PipeLine= "";
    Device  = "";
    CamName = "";
    CamWidth=0;
    CamHeight=0;

    std::string Jstr = s.at("VIDEO_INPUT");
    if(!Jstr.empty()){
        if(TJ.GetSetting(s["VIDEO_INPUTS_PARAMS"],Jstr,Device)){
            CamName = s.at("CAM_NAME");
            std::cout << "Start thread : " << CamName << std::endl;
            //start the device
            ThGr = std::thread([=] { GrabThread(); });
        }
        else{
            Device="";
            std::cout << "Error reading the json pipeline!" << std::endl;
        }
    }
    else{
        std::cout << "Error reading VIDEO_INPUT value!" << std::endl;
    }
}
//----------------------------------------------------------------------------------
void ThreadCam::Quit(void)
{
    GrabStop.store(true);
    ThGr.join();                //wait until the grab thread ends
}
//----------------------------------------------------------------------------------
void ThreadCam::GetPipeLine(void)
{
    int Wd, Ht, CC;
    char Acc[8];
    float ScaleW, ScaleH;

    cap.release();                                                           //close if open
    //test if the Device is a rtsp stream
    if((Device.find("rtsp")!= std::string::npos) || (Device.find("RTSP")!= std::string::npos) ){
        //found a rtsp Device. test if the command isn't already defined by the user
        if(Device.find("appsink")== std::string::npos){                      //an user string, not a full NVIDIA pipeline
            cap.open(Device);                                                //get new connection
            if(cap.isOpened()){
                CamWidth  = cap.get(cv::CAP_PROP_FRAME_WIDTH);               //get width of the stream
                CamHeight = cap.get(cv::CAP_PROP_FRAME_HEIGHT);              //get height of the stream
                CC = cap.get(cv::CAP_PROP_FOURCC);                           //get codec of the stream
                sprintf(Acc, "%c%c%c%c", int(CC&255) , int((CC>>8)&255), int((CC>>16)&255), int((CC>>24)&255) );
                CamCodec = Acc;                                              //convert to readable format

                ScaleW = (float)Width/CamWidth;
                ScaleH = (float)Height/CamHeight;
                if(ScaleW <= ScaleH){
                    if(ScaleW > 1.0){ Wd=Width; Ht=Height; }
                    else{ Wd=Width; Ht=ScaleW*CamHeight; }
                }
                else{
                    if(ScaleH > 1.0){ Wd=Width; Ht=Height; }
                    else{ Ht=Height; Wd=ScaleH*CamWidth; }
                }
                Wd = (Wd/4)*4;  Ht = (Ht/4)*4;                                  //align on 4

                if(CamCodec == "h264"){
                    PipeLine ="rtspsrc location="+Device;
                    PipeLine+=" latency=300 ! ";
                    PipeLine+="rtph264depay ! h264parse ! ";
                    PipeLine+="queue max-size-buffers=10 leaky=2 ! ";
                    PipeLine+="nvv4l2decoder ! video/x-raw(memory:NVMM) , format=(string)NV12 ! nvvidconv ! ";
                    PipeLine+="video/x-raw , width="+std::to_string(Wd);
                    PipeLine+=" , height="+std::to_string(Ht);
                    PipeLine+=" , format=(string)BGRx ! videoconvert ! appsink drop=1 sync=0";
                }
                else{
                    if(CamCodec == "h265"){
                        PipeLine ="rtspsrc location="+Device;
                        PipeLine+=" latency=300 ! ";
                        PipeLine+="rtph265depay ! h265parse ! ";
                        PipeLine+="queue max-size-buffers=10 leaky=2 ! ";
                        PipeLine+="nvv4l2decoder ! video/x-raw(memory:NVMM) , format=(string)NV12 ! nvvidconv ! ";
                        PipeLine+="video/x-raw , width="+std::to_string(Wd);
                        PipeLine+=" , height="+std::to_string(Ht);
                        PipeLine+=" , format=(string)BGRx ! videoconvert ! appsink drop=1 sync=0";
                    }
                    else{
                        //unknown codec, copy the Device to the pipeline (user pipeline)
                        PipeLine=Device;
                    }
                }
                //if you want to debug, go ahead
    //            std::cout << Dev << std::endl;
    //            std::cout << CamWidth  << std::endl;
    //            std::cout << CamHeight  << std::endl;
    //            std::cout << CamCodec  << std::endl;
    //            std::cout << Ratio  << std::endl;
    //            std::cout << ScaleW  << std::endl;
    //            std::cout << ScaleH  << std::endl;
    //            std::cout << Wd  << std::endl;
    //            std::cout << Ht  << std::endl;
    //            std::cout << PipeLine  << std::endl;
            }
            cap.release();                                      //close
        }
        else{
            //just copy the Device to the pipeline (user pipeline)
            PipeLine=Device;
        }
    }
    else{
        //just copy the Device to the pipeline (user pipeline)
        PipeLine=Device;
    }
}
//----------------------------------------------------------------------------------
void ThreadCam::GrabThread(void)
{
    int Try=0;
    cv::Mat tmp;


    while(!cap.isOpened()){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if(PipeLine=="") GetPipeLine();                 //get a proper pipeline (if not done yet)
        if(PipeLine!="") cap.open(PipeLine);            //open the Device with the new pipeline
    }
    GrabOpen.store(true);

    while(GrabStop.load() == false){
        if(!cap.read(tmp)){
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            Try++;
            if(Try>250){    //after 3.5 Sec no sound
                //lost connection, retry to open again.
                cap.release();
                GrabOpen.store(false);
                while(!cap.isOpened()){
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    if(PipeLine=="") GetPipeLine();                 //get a proper pipeline (if not done yet)
                    if(PipeLine!="") cap.open(PipeLine);            //open the Device with the new pipeline
                }
                GrabOpen.store(true);
                Try=0;
            }
        }
        else{
            std::lock_guard<std::mutex> lock(mtx);
            tmp.copyTo(mm);
            GrabTag.store(++Tag);
            Try=0;
        }
        //debug for mp4 movie to slow down
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}
//----------------------------------------------------------------------------------
bool ThreadCam::GetFrame(cv::Mat& m)
{
    bool Success = false;
    double Elapse;
    std::chrono::steady_clock::time_point Tyet, Tgrab;

    if(Device=="") return false;    //the pipeline wasn't even opened (wrong json entry?)

    //quick check to see if there is a new frame
    if(GrabTag.load()!=PrevTag){
        {
            std::lock_guard<std::mutex> lock(mtx);
            mm.copyTo(m);
            Success=true;
        }
        PrevTag=GrabTag.load();
    }
    else{
        //quick check to see if there is a connection
        if(GrabOpen.load()==false){
            //lost connection -> send empty frame
            Success=false;
        }
        else{
            //we have to wait (or the connection is lost)
            Tgrab = std::chrono::steady_clock::now();
            do{
                Tyet   = std::chrono::steady_clock::now();
                Elapse = std::chrono::duration_cast<std::chrono::milliseconds> (Tgrab - Tyet).count();
            }
            while(GrabTag.load()==PrevTag && GrabOpen.load()==true && Elapse<500); //wait 500mS before giving an empty frame

            if(GrabTag.load()!=PrevTag && GrabOpen.load()==true && Elapse<500){
                //new fram
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    mm.copyTo(m);
                    Success=true;
                }
                PrevTag=GrabTag.load();
            }
            else{
                //something wrong -> send empty frame
                Success=false;
            }
        }
    }
    return Success;
}
//----------------------------------------------------------------------------------
