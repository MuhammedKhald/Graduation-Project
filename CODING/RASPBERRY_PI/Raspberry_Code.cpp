#include <opencv2/opencv.hpp>
#include <raspicam_cv.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <wiringPi.h>

using namespace std;
using namespace cv;
using namespace raspicam;

// Image Processing variables
Mat frame, Matrix, framePers, frameGray, frameThresh, frameEdge, frameFinal, frameFinalDuplicate, frameFinalDuplicate1;
Mat ROILane, ROILaneEnd;
int LeftLanePos, RightLanePos, frameCenter, laneCenter, Result, laneEnd;

RaspiCam_Cv Camera;

stringstream ss;


vector<int> histrogramLane;
vector<int> histrogramLaneEnd;

Point2f Source[] = {Point2f(40,158),Point2f(360,158),Point2f(20,203), Point2f(380,203)};
Point2f Destination[] = {Point2f(100,0),Point2f(280,0),Point2f(100,240), Point2f(280,240)};


//Machine Learning variables
CascadeClassifier Stop_Cascade, Object_Cascade , Uturn_Cascade;

Mat frame_Stop, RoI_Stop, gray_Stop, frame_Object, RoI_Object, gray_Object, frame_Uturn, RoI_Uturn, gray_Uturn; //frame_TokTok, RoI_TokTok, gray_TokTok;
vector<Rect> Stop, Object,Uturn;
int dist_Stop, dist_Object, dist_Uturn ;

 void Setup ( int argc,char **argv, RaspiCam_Cv &Camera )
  {
    Camera.set ( CAP_PROP_FRAME_WIDTH,  ( "-w",argc,argv,400 ) );
    Camera.set ( CAP_PROP_FRAME_HEIGHT,  ( "-h",argc,argv,240 ) );
    Camera.set ( CAP_PROP_BRIGHTNESS, ( "-br",argc,argv,50 ) );
    Camera.set ( CAP_PROP_CONTRAST ,( "-co",argc,argv,50 ) );
    Camera.set ( CAP_PROP_SATURATION,  ( "-sa",argc,argv,50 ) );
    Camera.set ( CAP_PROP_GAIN,  ( "-g",argc,argv ,50 ) );
    Camera.set ( CAP_PROP_FPS,  ( "-fps",argc,argv,0));

}

void Capture()
{
    Camera.grab();
    Camera.retrieve( frame);
    cvtColor(frame, frame_Stop, COLOR_BGR2RGB);
    cvtColor(frame, frame_Object, COLOR_BGR2RGB);
     cvtColor(frame, frame_Uturn, COLOR_BGR2RGB);
    cvtColor(frame, frame, COLOR_BGR2RGB);
    
}

void Perspective()
{
	line(frame,Source[0], Source[1], Scalar(0,0,255), 2);
	line(frame,Source[1], Source[3], Scalar(0,0,255), 2);
	line(frame,Source[3], Source[2], Scalar(0,0,255), 2);
	line(frame,Source[2], Source[0], Scalar(0,0,255), 2);
	
	
	Matrix = getPerspectiveTransform(Source, Destination);
	warpPerspective(frame, framePers, Matrix, Size(400,240));
}

void Threshold()
{
	cvtColor(framePers, frameGray, COLOR_RGB2GRAY);
	inRange(frameGray,150,255, frameThresh);
	Canny(frameGray,frameEdge, 900, 900, 3, false);
	add(frameThresh, frameEdge, frameFinal);
	cvtColor(frameFinal, frameFinal, COLOR_GRAY2RGB);
	cvtColor(frameFinal, frameFinalDuplicate, COLOR_RGB2BGR);   //used in histrogram function only
	cvtColor(frameFinal, frameFinalDuplicate1, COLOR_RGB2BGR);   //used in histrogram function only
	
}

void Histrogram()
{
    histrogramLane.resize(400);
    histrogramLane.clear();
    
    for(int i=0; i<400; i++)       //frame.size().width = 400
    {
	ROILane = frameFinalDuplicate(Rect(i,140,1,100));
	divide(255, ROILane, ROILane);
	histrogramLane.push_back((int)(sum(ROILane)[0])); 
    }
	
	
}

void LaneFinder()
{
    vector<int>:: iterator LeftPtr;
    LeftPtr = max_element(histrogramLane.begin(), histrogramLane.begin() + 150);
    LeftLanePos = distance(histrogramLane.begin(), LeftPtr); 
    
    vector<int>:: iterator RightPtr;
    RightPtr = max_element(histrogramLane.begin() +250, histrogramLane.end());
    RightLanePos = distance(histrogramLane.begin(), RightPtr);
    
    line(frameFinal, Point2f(LeftLanePos, 0), Point2f(LeftLanePos, 240), Scalar(0, 255,0), 2);
    line(frameFinal, Point2f(RightLanePos, 0), Point2f(RightLanePos, 240), Scalar(0,255,0), 2); 
}

void LaneCenter()
{
    laneCenter = (RightLanePos-LeftLanePos)/2 +LeftLanePos;
    frameCenter = 192;
    
    line(frameFinal, Point2f(laneCenter,0), Point2f(laneCenter,240), Scalar(0,255,0), 3);
    line(frameFinal, Point2f(frameCenter,0), Point2f(frameCenter,240), Scalar(255,0,0), 3);

    Result = laneCenter-frameCenter;
}


void Stop_detection()
{
    if(!Stop_Cascade.load("/home/pi/Downloads/New_stop_Home100.xml"))
    {
	printf("Unable to open stop cascade file");
    }
    
    RoI_Stop = frame_Stop(Rect(0,0,300,240));
    cvtColor(RoI_Stop, gray_Stop, COLOR_RGB2GRAY);
    equalizeHist(gray_Stop, gray_Stop);
    Stop_Cascade.detectMultiScale(gray_Stop, Stop);
    
    for(int i=0; i<Stop.size(); i++)
    {
	Point P1(Stop[i].x, Stop[i].y);
	Point P2(Stop[i].x + Stop[i].width, Stop[i].y + Stop[i].height);
	
	rectangle(RoI_Stop, P1, P2, Scalar(0, 0, 255), 2);
	putText(RoI_Stop, "Stop Sign", P1, FONT_HERSHEY_PLAIN, 1,  Scalar(0, 0, 255, 255), 2);
	
	dist_Stop = (-0.866)*(P2.x-P1.x) + 74.2;
	
       ss.str(" ");
       ss.clear();
       ss<<"D = "<<dist_Stop<<" CM ";
       putText(RoI_Stop, ss.str(), Point2f(1,130), 0,1, Scalar(0,0,255), 2);
	
    }
}
    
    void Uturn_detection()
{
    if(!Uturn_Cascade.load("/home/pi/Downloads/uturn+.xml"))
    {
	printf("Unable to open stop cascade file");
    }
    
    RoI_Uturn = frame_Uturn(Rect(0,0,300,240));
    cvtColor(RoI_Uturn, gray_Uturn, COLOR_RGB2GRAY);
    equalizeHist(gray_Uturn, gray_Uturn);
    Uturn_Cascade.detectMultiScale(gray_Uturn, Uturn);
    
    for(int i=0; i<Uturn.size(); i++)
    {
	Point P1(Uturn[i].x, Uturn[i].y);
	Point P2(Uturn[i].x + Uturn[i].width, Uturn[i].y + Uturn[i].height);
	
	rectangle(RoI_Uturn, P1, P2, Scalar(0, 0, 255), 2);
	putText(RoI_Uturn, "U-turn", P1, FONT_HERSHEY_PLAIN, 1,  Scalar(0, 0, 255, 255), 2);
	
	dist_Uturn = (-0.866)*(P2.x-P1.x) + 74.2;
	
       ss.str(" ");
       ss.clear();
       ss<<"D = "<<dist_Uturn<<" CM ";
       putText(RoI_Uturn, ss.str(), Point2f(1,130), 0,1, Scalar(0,0,255), 2);
	
    }
    
}


void Object_detection()
{
    if(!Object_Cascade.load("/home/pi/Downloads/geshcar20more.xml"))
    {
	printf("Unable to open Object cascade file");
    }
    
    RoI_Object = frame_Object(Rect(100,50,200,190));
    cvtColor(RoI_Object, gray_Object, COLOR_RGB2GRAY);
    equalizeHist(gray_Object, gray_Object);
    Object_Cascade.detectMultiScale(gray_Object, Object);
    
    for(int i=0; i<Object.size(); i++)
    {
	Point P1(Object[i].x, Object[i].y);
	Point P2(Object[i].x + Object[i].width, Object[i].y + Object[i].height);
	
	rectangle(RoI_Object, P1, P2, Scalar(0, 0, 255), 2);
	putText(RoI_Object, "Object", P1, FONT_HERSHEY_PLAIN, 1,  Scalar(0, 0, 255, 255), 2);
	dist_Object = (-0.411)*(P2.x-P1.x) + 59.411;
	
       ss.str(" ");
       ss.clear();
       ss<<"D = "<< dist_Object<<"cm";
       putText(RoI_Object, ss.str(), Point2f(1,130), 0,1, Scalar(0,0,255), 2);
	
    }
}
   
 
    
    


int main(int argc,char **argv)
{
	
    wiringPiSetup();
    pinMode(21, OUTPUT);
    pinMode(22, OUTPUT);
    pinMode(23, OUTPUT);
    pinMode(24, OUTPUT);
    
    Setup(argc, argv, Camera);
    cout<<"Connecting to camera"<<endl;
    if (!Camera.open())
    {
		
	cout<<"Failed to Connect"<<endl;
    }
     
	cout<<"Camera Id = "<<Camera.getId()<<endl;
     
 
    while(1)
    {
	
    auto start = std::chrono::system_clock::now();

    Capture();
    Perspective();
    Threshold();
    Histrogram();
    LaneFinder();
    LaneCenter();
    Stop_detection();
    Object_detection();
    Uturn_detection();
   // TokTok_detection();
    
    if (dist_Stop==17 || dist_Stop==19 ||dist_Stop==20)
    {
	digitalWrite(21, 0);
	digitalWrite(22, 0);    //decimal = 8
	digitalWrite(23, 0);
	digitalWrite(24, 1);
	cout<<"Stop Sign"<<endl;
	dist_Stop = 0;
	
	goto Stop_Sign;
	
	
	
    }
    
        if (dist_Object==30 || dist_Object==31 ||dist_Object==29)
   {
	digitalWrite(21, 1);
	digitalWrite(22, 0);    //decimal = 9
	digitalWrite(23, 0);
	digitalWrite(24, 1);
	cout<<"Object"<<endl;
	dist_Object = 0;
	
	goto Object;
	
	
	}
	  if (dist_Uturn >=20&& dist_Uturn<=23)
    {
	
       	digitalWrite(21, 1);
	digitalWrite(22, 1);    //decimal = 7
	digitalWrite(23, 1);
	digitalWrite(24, 0);
	cout<<"Uturn"<<endl;
	dist_Uturn = 0;
	goto Uturn;
	
	}
	
    

 /*  if( lane ==8600);
    {
       	digitalWrite(21, 1);
	digitalWrite(22, 1);    //decimal = 7
	digitalWrite(23, 1);
	digitalWrite(24, 0);
	cout<<"Lane End"<<endl;
    }*/
    
    
    if (Result ==0)
    {
	digitalWrite(21, 0);
	digitalWrite(22, 0);    //decimal = 0
	digitalWrite(23, 0);
	digitalWrite(24, 0);
	cout<<"Forward"<<endl;
    }
    
        
    else if (Result >0 && Result<= 7)
    {
	digitalWrite(21, 1);
	digitalWrite(22, 0);    //decimal = 1
	digitalWrite(23, 0);
	digitalWrite(24, 0);
	cout<<"Right1"<<endl;
    }
    
        else if (Result >7 && Result <=12)
    {
	digitalWrite(21, 0);
	digitalWrite(22, 1);    //decimal = 2
	digitalWrite(23, 0);
	digitalWrite(24, 0);
	cout<<"Right2"<<endl;
    }
    
        else if (Result >12)
    {
	digitalWrite(21, 1);
	digitalWrite(22, 1);    //decimal = 3
	digitalWrite(23, 0);
	digitalWrite(24, 0);
	cout<<"Right3"<<endl;
    }
    
        else if (Result < 0 && Result >=-7)
    {
	digitalWrite(21, 0);
	digitalWrite(22, 0);    //decimal = 4
	digitalWrite(23, 1);
	digitalWrite(24, 0);
	cout<<"Forward"<<endl;
    }

        else if (Result< -7 && Result >=-12)
    {
	digitalWrite(21, 1);
	digitalWrite(22, 0);    //decimal = 5
	digitalWrite(23, 1);
	digitalWrite(24, 0);
	cout<<"Left2"<<endl;
    }
    
        else if (Result <-12)
    {
	digitalWrite(21, 0);
	digitalWrite(22, 1);    //decimal = 6
	digitalWrite(23, 1);
	digitalWrite(24, 0);
	cout<<"Left3"<<endl;
    }
    Stop_Sign:
    Object:
    Uturn:
    
  
   
  
    
   

    
  /* if (laneEnd >8600)
    {
       ss.str(" ");
       ss.clear();
       ss<<" Lane End";
       putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(255,0,0), 2);
    
     }*/
    
if (Result == 0)
    {
       ss.str(" ");
       ss.clear();
       ss<<"Result = "<<Result<<" (Move Forward)";
       putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(0,0,255), 2);
    
     }
    
    else if (Result > 0)
    {
       ss.str(" ");
       ss.clear();
       ss<<"Result = "<<Result<<" (Move Right)";
       putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(0,0,255), 2);
    
     }
     
     else if (Result < 0)
    {
       ss.str(" ");
       ss.clear();
       ss<<"Result = "<<Result<<" (Move Left)";
       putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(0,0,255), 2);
    
     }
    
    
    namedWindow("orignal", WINDOW_KEEPRATIO);
    moveWindow("orignal", 0, 100);
    resizeWindow("orignal", 640, 480);
    imshow("orignal", frame);
    
    namedWindow("Perspective", WINDOW_KEEPRATIO);
    moveWindow("Perspective", 640, 100);
    resizeWindow("Perspective", 640, 480);
    imshow("Perspective", framePers);
    
    namedWindow("Final", WINDOW_KEEPRATIO);
    moveWindow("Final", 1280, 100);
    resizeWindow("Final", 640, 480);
    imshow("Final", frameFinal);
    
    namedWindow("Stop Sign", WINDOW_KEEPRATIO);
    moveWindow("Stop Sign", 1280, 580);
    resizeWindow("Stop Sign", 640, 480);
    imshow("Stop Sign", RoI_Stop);
    
    namedWindow("Object", WINDOW_KEEPRATIO);
    moveWindow("Object", 640, 580);
    resizeWindow("Object", 640, 480);
    imshow("Object", RoI_Object);
   
    namedWindow("Uturn", WINDOW_KEEPRATIO);
    moveWindow("Uturn",0, 580);
    resizeWindow("Uturn", 640, 480);
    imshow("Uturn", RoI_Uturn); 
    
    
    
    waitKey(1);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    
    float t = elapsed_seconds.count();
    int FPS = 1/t;
    //cout<<"FPS = "<<FPS<<endl;
    
    }

    
    return 0;
}

     

