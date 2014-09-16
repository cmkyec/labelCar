#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <io.h>
#include <Windows.h>
#include <opencv2\opencv.hpp>
#include "json\include\json.h"

static cv::Mat g_showImg, g_srcImg;
static cv::Point g_prevPt, g_curPt;
char g_savepath[100];
std::string g_folder;
int g_index = 1;
bool g_bnextImage = false;

struct param
{
	std::string imagesFolder;
	std::string postfix;
	std::string carsFolder;
};

bool readConfig(std::string configFilePath, struct param& param)
{
	std::ifstream configFile(configFilePath.c_str());
	if (!configFile.is_open()) {
		std::cerr << "can not open the config file: " << configFilePath << std::endl;
		return false;
	}
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(configFile, root)) {
		std::string errMsg = reader.getFormatedErrorMessages();
		std::cerr << "parse config file failed." << std::endl;
		std::cerr << errMsg << std::endl;
		return false;
	}

	param.imagesFolder = root["imagesFolder"].asString();
	param.postfix = root["postfix"].asString();
	param.carsFolder = root["carsFolder"].asString();
	return true;
}

bool createCarsFolder(std::string carsFolder)
{
	BOOL res = ::CreateDirectory(carsFolder.c_str(), NULL);
	if (!res && ::GetLastError() != ERROR_ALREADY_EXISTS) {
		std::cerr << "create cars folder failed. please check the cars folder path."
			  << std::endl;
		return false;
	}
	return true;
}

int traverse(std::string folder, std::string postfix,
	     std::vector<std::string>& paths)
{
	std::string wildPath = folder + "\\*." + postfix;
	_finddata_t fileInfo;
	intptr_t handle = _findfirst(wildPath.c_str(), &fileInfo);

	if (handle == -1L) {
		std::cout << "there is no file with the postfix in the folder."
			<< std::endl;
		return 0;
	}
	do {
		std::string path = folder + "\\" + fileInfo.name;
		paths.push_back(path);
	} while (_findnext(handle, &fileInfo) == 0);

	_findclose(handle);

	return int(paths.size());
}

static void onMouse(int event, int x, int y, int flag, void*)
{
	static cv::Mat showImgTmp;
	if (g_showImg.empty()) return;
	if (x < 0 || x >= g_showImg.cols || y < 0 || y >= g_showImg.rows) return;
	if (event == CV_EVENT_LBUTTONUP && !(flag & CV_EVENT_FLAG_LBUTTON)) {
		g_curPt = cv::Point(x, y);
		g_srcImg.copyTo(g_showImg);
		cv::rectangle(g_showImg, g_prevPt, g_curPt, cv::Scalar(0, 255, 255), 2);
	}
	if (event == CV_EVENT_LBUTTONDOWN) {
		g_prevPt = cv::Point(x, y);
	}
	if (event == CV_EVENT_MOUSEMOVE && (flag & CV_EVENT_FLAG_LBUTTON)) {
		g_curPt = cv::Point(x, y);
		g_srcImg.copyTo(g_showImg);
		cv::rectangle(g_showImg, g_prevPt, g_curPt, cv::Scalar(0, 0, 255), 2);
	}
	if (event == CV_EVENT_RBUTTONDOWN && (flag & CV_EVENT_FLAG_RBUTTON)) {
		if (g_prevPt != cv::Point(-1, -1) && g_curPt != cv::Point(-1, -1)) {
			sprintf_s(g_savepath, sizeof(g_savepath), "%s\\%05d_sample.png",
				g_folder.c_str(), g_index++);
			cv::Rect rect(g_prevPt, g_curPt);
			cv::imwrite(g_savepath, g_srcImg(rect));
			g_bnextImage = true;
		}
		g_prevPt = g_curPt = cv::Point(-1, -1);
	}
}

int main(int argc, char** argv)
{
	std::string configFilePath;
	if (argc < 2) {
		configFilePath = "./labelCar.config";
		std::cout << "use the default config file: ./labelCar.config" << std::endl;
		std::cout << "specify the config file by labelCar.exe path_to_config_file"
			  << std::endl;
	}
	else {
		configFilePath = argv[1];
	}

	struct param param;
	if (!readConfig(configFilePath, param)) return -1;
	std::cout << "images folder: " << param.imagesFolder << std::endl;
	std::cout << "postfix: " << param.postfix << std::endl;
	std::cout << "cars folder: " << param.carsFolder << std::endl;

	if (!createCarsFolder(param.carsFolder)) return -1;
	g_folder = param.carsFolder;

	std::vector<std::string> imgsPath;
	if (traverse(param.imagesFolder, param.postfix, imgsPath) == 0) {
		std::cout << "there are no images in the images folder." << std::endl;
		return -1;
	}
	
	cv::namedWindow("show", 0);
	cv::setMouseCallback("show", onMouse, NULL);
	char process[100];
	memset(process, 0, sizeof(process));
	for (std::size_t i = 0; i < imgsPath.size(); ++i) {
		g_srcImg = cv::imread(imgsPath[i]);
		g_bnextImage = false;
		sprintf_s(process, sizeof(process), "%d/%d", (int)i + 1, (int)imgsPath.size());
		cv::putText(g_srcImg, process, cv::Point(50, 50), cv::FONT_HERSHEY_PLAIN, 2.0, 
			cv::Scalar(0, 0, 255));
		g_srcImg.copyTo(g_showImg);
		while (true) {
			cv::imshow("show", g_showImg);
			if (g_bnextImage) break;
			int c = cv::waitKey(30);
			if (c == 'q' || c == 'Q') {
				g_prevPt = g_curPt = cv::Point(-1, -1);
				break;
			}
			if (c == 'r' || c == 'R') {
				g_srcImg.copyTo(g_showImg);
				g_prevPt = g_curPt = cv::Point(-1, -1);
			}
			else continue;
		}
	}
	cv::destroyWindow("show");

	return 0;
}