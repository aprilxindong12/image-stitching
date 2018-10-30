#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <numeric>
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "timer_functions.h"

using namespace std;
using namespace cv;
using namespace cv::detail;

typedef struct {
	double rotation_angle;
	double scaling;
	int no_of_rows;
	int no_of_inliers;
	Mat result;
	Mat mask;	
	bool match;
	bool original;
	bool straight;
	bool forward;
} simpleStitchReturn;

// Default command line args
string dir;
string image_ext;
bool stitchForward = true;
double work_megapix = 0.6;

int max_no_of_stitched_parts = 10;
int no_of_overlapping_frames = 50;
int max_no_of_mismatches = 10;
int min_no_of_rows = 1;
int min_no_of_inliers_1 = 8;
double match_conf_1 = 0.3;
double scaling_range_1 = 0.1;

int min_no_of_inliers_2 = 8;
double match_conf_2 = 0.2;
double scaling_range_2 = 0.2;
double roi_rows = 2;

// other global variables
int min_no_of_inliers = 8;
double match_conf = 0.3;
double scaling_range = 0.1;
int roi_rows_0 = 100;
int roi_rows_1 = 100;
int start_0 = 0;


static void printUsage()
{
	cout <<
		"Stitch a sequence of images of the same size forwards or backwards.\n\n"
		"stitch dir [flags]\n\n"
		"Flags:\n"
		"  --image_ext (bmp|jpg|png|...)\n"
		"		imread supported file formats. If not specified, all files in the folder will be read.\n"
		"  --stitchForward <int>\n"
		"		1 for stitch images forwards, 0 for backwards. The default is 1.\n"
		"  --work_megapix <float>\n"
		"		Resolution for image registration step. The default is 0.6 Mpx.\n"

		"Stitching input images:\n"
		"  --max_no_of_stitched_parts <int>\n"
		"		Maximum number of separately stitched parts from input images. The default is 10.\n"
		"  --no_of_overlapping_frames <int>\n"
		"		Number of overlapping frames between two successively stitched parts. The default is 50.\n"
		"  --max_no_of_mismatches <int>\n"
		"		Allowable number of consecutively unmatched images. The default is 10.\n"
		"  --min_no_of_rows <int>\n"
		"		Minimum number of new rows of each image to be stitched. The default is 1.\n"
		"  --min_no_of_inliers_1 <int>\n"
		"		Minimum number of geometrically consistent matches between two images. The default is 8.\n"
		"  --match_conf_1 <float>\n"
		"		Confidence for feature matching between two input images. The default is 0.3.\n"
		"  --scaling_range_1 <int>\n"
		"		Allowed scaling tolerance when transforming one image to match the other. The default is 0.1./\n"

		"Stitching stitched parts:\n"
		"  --min_no_of_inliers_2 <int>\n"
		"		Minimum number of geometrically consistent matches between two stitched parts. The default is 8.\n"
		"  --match_conf_2 <float>\n"
		"		Confidence for feature matching between two stitched parts. The default is 0.2.\n"
		"  --scaling_range_2 <int>\n"
		"		Allowed scaling tolerance when transforming one stitched part to match the other. The default is 0.2./\n"
		"  --roi_rows <float> \n"
		"		Relative region of interest for finding features in two sticthed parts. The default is 2.0.\n";
}

static int parseCmdArgs(int argc, char** argv)
{
	if (argc == 1)
	{
		printUsage();
		return -1;
	}
	for (int i = 1; i < argc; ++i)
	{
		if (string(argv[i]) == "--help" || string(argv[i]) == "/?")
		{
			printUsage();
			return -1;
		}
		else if (string(argv[i]) == "--image_ext")
		{
			image_ext = argv[i + 1];
			i++;
		}
		else if (string(argv[i]) == "--stitchForward")
		{
			if (atoi(argv[i + 1]) == 0 | atoi(argv[i + 1]) == 1)
				stitchForward = atoi(argv[i + 1]);
			else
			{
				cout << "Bad stitchFoward flag.\n";
				cout << "Enter a value of 0 or 1.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--work_megapix")
		{
			work_megapix = atof(argv[i + 1]);
			i++;
		}
		

		else if (string(argv[i]) == "--max_no_of_stitched_parts")
		{
			if (atoi(argv[i + 1]) >= 1)
				max_no_of_stitched_parts = atoi(argv[i + 1]);
			else
			{
				cout << "Bad max_no_of_stitched_parts.\n";
				cout << "Enter a value >= 1.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--no_of_overlapping_frames")
		{
			if (atoi(argv[i + 1]) >= 0)
				no_of_overlapping_frames = atoi(argv[i + 1]);
			else
			{
				cout << "Bad no_of_overlapping_frames.\n";
				cout << "Enter a value >= 0.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--max_no_of_mismatches")
		{
			if (atoi(argv[i + 1]) >= 0)
				max_no_of_mismatches = atoi(argv[i + 1]);
			else
			{
				cout << "Bad max_no_of_mismatches.\n";
				cout << "Enter a value >= 0.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--min_no_of_rows")
		{
			if (atoi(argv[i + 1]) >= 1)
				min_no_of_rows = atoi(argv[i + 1]);
			else
			{
				cout << "Bad min_no_of_rows.\n";
				cout << "Enter a value >= 1.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--min_no_of_inliers_1")
		{
			if (atoi(argv[i + 1]) >= 4)
				min_no_of_inliers_1 = atoi(argv[i + 1]);
			else
			{
				cout << "Bad min_no_of_inliers_1.\n";
				cout << "Enter a value >= 4.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--match_conf_1")
		{
			match_conf_1 = atof(argv[i + 1]);
			i++;
		}
		else if (string(argv[i]) == "--scaling_range_1")
		{
			scaling_range_1 = atof(argv[i + 1]);
			i++;
		}

		else if (string(argv[i]) == "--min_no_of_inliers_2")
		{
			if (atoi(argv[i + 1]) >= 4)
				min_no_of_inliers_2 = atoi(argv[i + 1]);
			else
			{
				cout << "Bad min_no_of_inliers_2.\n";
				cout << "Enter a value >= 4.\n";
				return -1;
			}
			i++;
		}
		else if (string(argv[i]) == "--match_conf_2")
		{
			match_conf_2 = atof(argv[i + 1]);
			i++;
		}
		else if (string(argv[i]) == "--roi_rows")
		{
			roi_rows = atof(argv[i + 1]);
			i++;
		}
		else if (string(argv[i]) == "--scaling_range_2")
		{
			scaling_range_2 = atof(argv[i + 1]);
			i++;
		}
		else
			dir = argv[i];
	}
	if (dir.empty())
	{
		printUsage();
		return -1;
	}

	return 0;
}

bool compLess(String S1, String S2)
{
	if (S1.length() == S2.length())
		return S1 < S2;
	else
		return S1.length() < S2.length();
}

bool compGreater(String S1, String S2)
{
	if (S1.length() == S2.length())
		return S1 > S2;
	else
		return S1.length() > S2.length();
}

bool IsNegative(int i) { return (i < 0); }

simpleStitchReturn simpleStitchTransparent(vector<Mat> images, vector<Mat> masks);

int main(int argc, char* argv[])
{
	int retval = parseCmdArgs(argc, argv);
	if (retval)
		return retval;

	String dirname;
	if (image_ext.empty())
		dirname = dir + String("/*");
	else
		dirname = dir + String("/*.") + image_ext;

	vector<String> files;
	vector<Mat> full_images(2), images(2), masks(2);
	int counter = 0;
	double work_scale;
	int standard_rows;
	Mat result_image, result_image_bgra, result_mask;
	simpleStitchReturn res;

	// get all image file names and sort
	glob(dirname, files);
	
	int no_of_inputs = files.size();
	if (no_of_inputs < 2)
	{
		std::cout << "Need more images.\n";
		return -1;
	}

	if (stitchForward)
		sort(files.begin(), files.end(), compLess);
	else
		sort(files.begin(), files.end(), compGreater);

	vector<double> rotation_angle(no_of_inputs, 0);
	vector<double> scaling(no_of_inputs, 1);
	vector<int> no_of_rows(no_of_inputs, -1);
	vector<int> no_of_inliers(no_of_inputs, 0);

	stopWatch s;
	startTimer(&s);

	// continuous stitching-------------------------------------------------------------
	vector<Mat> stitched_images(max_no_of_stitched_parts);
	vector<Mat> stitched_image_masks(max_no_of_stitched_parts);
	vector<int> total_height(max_no_of_stitched_parts);
	vector<int> actual_no_of_frames(max_no_of_stitched_parts);
	vector<int> no_of_frames(max_no_of_stitched_parts);
	int total_no_of_frames = 0;
	int counter2 = 0;
	int n = 0;
	int idx;

	// read a valid image and set some parameters
	full_images[1] = imread(files[n]);
	while (full_images[1].empty() && n < no_of_inputs)
	{
		std::cout << files[n] << " is invalid!" << endl;
		n++;
		full_images[1] = imread(files[n]);
	}

	work_scale = min(1.0, sqrt(work_megapix * 1e6 / full_images[1].size().area()));
	cv::resize(full_images[1], images[1], Size(), work_scale, work_scale, INTER_AREA);

	standard_rows = images[1].rows;
	roi_rows_0 = standard_rows;
	roi_rows_1 = standard_rows;
	start_0 = 0;
	match_conf = match_conf_1;
	min_no_of_inliers = min_no_of_inliers_1;
	scaling_range = scaling_range_1;

	n = 0;
	while (total_no_of_frames < no_of_inputs && counter2 < max_no_of_stitched_parts)
	{
		std::cout << "Stitching part " << (counter2 + 1) << "...\n";

		// read first valid image
		full_images[1] = imread(files[n]);
		while (full_images[1].empty() && n < no_of_inputs)
		{
			std::cout << files[n] << " is invalid!" << endl;
			n++;
			full_images[1] = imread(files[n]);
		}

		no_of_rows[n] = full_images[1].rows;
		total_height[counter2] = no_of_rows[n];
		rotation_angle[n] = 0;
		scaling[n] = 1;
		no_of_inliers[n] = 0;

		cv::resize(full_images[1], images[1], Size(), work_scale, work_scale, INTER_AREA);
		
		masks[1].create(images[1].size(), CV_8U);	
		masks[1].setTo(Scalar::all(255));
		masks[0].create(images[1].size(), CV_8U);
		
		for (idx = n + 1; idx < no_of_inputs; ++idx)
		{
			
			full_images[0] = imread(files[idx]);
			if (full_images[0].empty())
			{
				std::cout << files[idx] << " is invalid!" << endl;
			}
			else
			{
				cv::resize(full_images[0], images[0], Size(), work_scale, work_scale, INTER_AREA);				
				masks[0].setTo(Scalar::all(255));

				res = simpleStitchTransparent(images, masks);
				no_of_inliers[idx] = res.no_of_inliers;

				if (res.match)
				{
					counter = 0;					
					rotation_angle[idx] = res.rotation_angle;
					scaling[idx] = res.scaling;

					if (res.original)
					{
						if (res.straight)
						{
							if (res.forward)
							{
								images[1] = res.result;
								masks[1] = res.mask;
								no_of_rows[idx] = cvRound(res.no_of_rows / work_scale);
								total_height[counter2] += no_of_rows[idx];
							}
							else
								no_of_rows[idx] = 0;
						}
						else
							no_of_rows[idx] = -2;
					}
					else
						no_of_rows[idx] = -3;
				}
				else
				{
					counter++;
				}

				actual_no_of_frames[counter2] = no_of_inputs - n - counter;

				if (counter == max_no_of_mismatches + 1)
				{
					actual_no_of_frames[counter2] = idx - n - max_no_of_mismatches;
					break;
				}
			}
		}
		
		std::cout << "Stitched " << actual_no_of_frames[counter2] << " images.\n";
		no_of_frames[counter2] = actual_no_of_frames[counter2];
		total_no_of_frames += actual_no_of_frames[counter2];

		// going back no_of_overlapping_frames images
		n = total_no_of_frames;
		if (total_no_of_frames < no_of_inputs && no_of_overlapping_frames > 0 && (counter2 + 1) < max_no_of_stitched_parts)
		{
			int m = 0;
			for (n = total_no_of_frames - 1; n >= 0; n--)
			{
				if (no_of_rows[n] > 0)
				{
					m++;
					if (m == no_of_overlapping_frames)
						break;
				}
			}

			if (n == 0)
			{
				n = total_no_of_frames;
				std::cout << "Will not be able to stitch part " << (counter2 + 2) << " to the other parts.\n";
				std::cout << "Try decreasing no_of_overlapping_frames.\n";
			}
			else
			{
				no_of_frames[counter2] = actual_no_of_frames[counter2] - (total_no_of_frames - n);
				total_no_of_frames = n;
				std::cout << "Going back to image " << n + 1 << endl;
			}			
		}

		stitched_images[counter2] = images[1];
		stitched_image_masks[counter2] = masks[1];

		cv::resize(images[1], result_image, Size(), 1. / work_scale, 1. / work_scale, INTER_CUBIC);
		string img_name = "part_" + to_string(counter2 + 1) + ".bmp";
		cv::imwrite(img_name, result_image);

		counter2++;
	}

	std::cout << "Completed stitching " << total_no_of_frames << " images.\n";
	std::cout << "Total of " << counter2 << " stitched parts.\n\n";

	// stitch partial images together------------------------------------------------------------------
	vector<int> new_no_of_frames(no_of_frames.begin(), no_of_frames.begin() + counter2);
	vector<int> accu_no_of_rows(no_of_inputs, -1);
	vector<int> new_no_of_rows(counter2 + 1, -1);
	vector<double> new_rotation_angle(counter2 + 1, 0);
	vector<double> new_scaling(counter2 + 1, 1);
	vector<int> new_no_of_inliers(counter2 + 1, 0);
	vector<int> effective_parts(counter2 + 1, 0);
	int new_total_height;

	vector<int> no_of_rows_copy(no_of_rows.begin(), no_of_rows.end());
	std::replace_if(no_of_rows_copy.begin(), no_of_rows_copy.end(), IsNegative, 0);

	vector<int> new_no_of_frames_accu(new_no_of_frames.size());
	partial_sum(new_no_of_frames.begin(), new_no_of_frames.end(), new_no_of_frames_accu.begin());

	if (counter2 > 1)
	{
		// read last valid image
		int n = total_no_of_frames - 1;
		while (no_of_rows[n] <= 0 && n >= 0)
		{
			n--;
		}

		full_images[1] = imread(files[n]);

		new_no_of_rows[counter2] = full_images[1].rows;
		new_total_height = new_no_of_rows[counter2];
		if (stitchForward)
			effective_parts[counter2] = n + 1;
		else
			effective_parts[counter2] = no_of_inputs - n;

		cv::resize(full_images[1], images[1], Size(), work_scale, work_scale, INTER_AREA);

		// flip the image		
		cv::flip(images[1], images[1], -1);
		masks[1].create(images[1].size(), CV_8U);
		masks[1].setTo(Scalar::all(255));

		match_conf = match_conf_2;
		min_no_of_inliers = min_no_of_inliers_2;
		scaling_range = scaling_range_2;
		roi_rows_1 = cvRound(standard_rows * roi_rows);
		roi_rows_0 = cvRound(standard_rows * roi_rows);

		std::cout << "Stitching partial images together...\n";
		
		int current_location = total_no_of_frames;
		for (int i = counter2 - 1; i >= 0; --i)
		{			
			cv::flip(stitched_images[i], images[0], -1);
			cv::flip(stitched_image_masks[i], masks[0], -1);

			if (images[1].rows < roi_rows_1)
				roi_rows_1 = images[1].rows;
			if (images[0].rows < roi_rows_0)
				roi_rows_0 = images[0].rows;
			
			start_0 = images[0].rows - roi_rows_0;

			res = simpleStitchTransparent(images, masks);			
			new_no_of_inliers[i] = res.no_of_inliers;

			if (res.match)
			{
				new_rotation_angle[i] = res.rotation_angle;
				new_scaling[i] = res.scaling;

				if (res.original)
				{
					if (res.straight)
					{
						if (res.forward)
						{
							images[1] = res.result;
							masks[1] = res.mask;
							new_no_of_rows[i] = cvRound(res.no_of_rows / work_scale);
							new_total_height += new_no_of_rows[i];
							effective_parts[i] = i + 1;
						}
						else
						{
							new_no_of_rows[i] = 0;
							std::cout << "No new rows in stitched part " << (i + 1) << endl;
						}
					}
					else
					{
						new_no_of_rows[i] = -2;
						std::cout << "Stitched part " << (i + 1) << " is out of the allowed range of rotation ";
						std::cout << "or estimated transformation is incorrect.\n";
						std::cout << "Try increasing match_conf_2 or increasing no_of_overlapping_frames.\n";
					}
				}
				else
				{
					new_no_of_rows[i] = -3;
					std::cout << "Estimated transformation of stitched part " << (i + 1) << " is out of the allowed range of scaling.\n";
				}
			}
			else
			{
				std::cout << "Not enough matches in stitched part " << (i + 1) << endl;
				std::cout << "Try increasing no_of_overlapping_frames or increasing roi_rows ";
				std::cout << "or decreasing match_conf_2 or decreasing min_no_of_inliers_2.\n";
			}

			roi_rows_1 = cvRound(standard_rows * roi_rows);
			roi_rows_0 = cvRound(standard_rows * roi_rows);

			// calculate accumulated rows in stitched parts
			current_location -= no_of_frames[i];
			std::partial_sum(no_of_rows_copy.begin() + current_location, no_of_rows_copy.begin() + current_location + no_of_frames[i], accu_no_of_rows.begin() + current_location);
		}
	}
	else
	{
		effective_parts[0] = 1;
		new_total_height = cvRound(images[1].rows / work_scale);
		std::partial_sum(no_of_rows_copy.begin(), no_of_rows_copy.end(), accu_no_of_rows.begin());
	}
	
	vector<int> new_no_of_rows_copy(new_no_of_rows.begin(), new_no_of_rows.end());
	std::replace_if(new_no_of_rows_copy.begin(), new_no_of_rows_copy.end(), IsNegative, 0);

	vector<int> new_no_of_rows_accu(new_no_of_rows_copy.size());
	reverse(new_no_of_rows_copy.begin(), new_no_of_rows_copy.end());
	partial_sum(new_no_of_rows_copy.begin(), new_no_of_rows_copy.end(), new_no_of_rows_accu.begin());
	reverse(new_no_of_rows_accu.begin(), new_no_of_rows_accu.end());

	// crop out edges
	threshold(masks[1], masks[1], 225, 255, cv::THRESH_BINARY);
	std::vector<cv::Point> points;
	for (cv::Mat_<uchar>::iterator it = masks[1].begin<uchar>(); it != masks[1].end<uchar>(); ++it)
		if (*it)
			points.push_back(it.pos());

	Rect roi = boundingRect(points);
	masks[1] = masks[1](roi);
	images[1] = images[1](roi);

	// set a transparent background
	Mat image_bgra, mask, channels[4];
	cv::flip(images[1], images[1], -1);
	cv::flip(masks[1], masks[1], -1);

	cv::cvtColor(images[1], image_bgra, CV_BGR2BGRA);
	masks[1].convertTo(mask, CV_8UC1);
	cv::split(image_bgra, channels);
	channels[3] &= mask;
	cv::merge(channels, 4, image_bgra);

	cv::resize(images[1], result_image, Size(), 1. / work_scale, 1. / work_scale, INTER_CUBIC);
	cv::resize(image_bgra, result_image_bgra, Size(), 1. / work_scale, 1. / work_scale, INTER_CUBIC);
	cv::resize(masks[1], result_mask, Size(), 1. / work_scale, 1. / work_scale, INTER_CUBIC);

	cv::imwrite("result.bmp", result_image);
	cv::imwrite("result_transparent.png", result_image_bgra);
	cv::imwrite("result_mask.bmp", result_mask);	

	stopTimer(&s);

	std::cout << "\nWriting to files...\n";
	std::ofstream csvfile;
	csvfile.open("csv_outputs.csv");
	csvfile << "image_no, no_of_rows, accu_no_of_rows, rotation_angle, scaling, no_of_inliers, ,";
	csvfile <<  "new_no_of_rows, new_no_of_rows_accu, new_rotation_angle, new_scaling, new_no_of_inliers\n";

	if (stitchForward)
	{
		for (int i = 0; i <= counter2; i++)
		{
			csvfile << to_string(i + 1) << "," << no_of_rows[i] << "," << accu_no_of_rows[i] << "," << rotation_angle[i] << "," << scaling[i] << "," << no_of_inliers[i] << ", ,";
			csvfile << new_no_of_rows[i] << "," << new_no_of_rows_accu[i] << "," << new_rotation_angle[i] << "," << new_scaling[i] << "," << new_no_of_inliers[i] << endl;
		}
		for (int i = counter2 + 1; i < no_of_inputs; i++)
		{
			csvfile << to_string(i + 1) << "," << no_of_rows[i] << "," << accu_no_of_rows[i] << "," << rotation_angle[i] << "," << scaling[i] << "," << no_of_inliers[i] << endl;
			
		}
	}
	else
	{
		for (int i = 0; i <= counter2; i++)
		{
			csvfile << to_string(no_of_inputs - i) << "," << no_of_rows[i] << "," << accu_no_of_rows[i] << "," << rotation_angle[i] << "," << scaling[i] << "," << no_of_inliers[i] << ", ,";
			csvfile << new_no_of_rows[i] << "," << new_no_of_rows_accu[i] << "," << new_rotation_angle[i] << "," << new_scaling[i] << "," << new_no_of_inliers[i] << endl;
		}
		for (int i = counter2 + 1; i < no_of_inputs; i++)
		{
			csvfile << to_string(no_of_inputs - i) << "," << no_of_rows[i] << "," << accu_no_of_rows[i] << "," << rotation_angle[i] << "," << scaling[i] << "," << no_of_inliers[i] << endl;
		}
	}		
	csvfile.close();

	cv::FileStorage outputs("outputs.xml", cv::FileStorage::WRITE);
	outputs << "FORWARD" << stitchForward;
	outputs << "WORK_SCALE" << work_megapix;

	outputs << "MAX_NO_OF_STITCHED_PARTS" << max_no_of_stitched_parts;
	outputs << "NO_OF_OVERLAPPING_FRAMES" << no_of_overlapping_frames;
	outputs << "MAX_NO_OF_MISMATCHES" << max_no_of_mismatches;
	outputs << "MIN_NO_OF_ROWS" << min_no_of_rows;	
	outputs << "MIN_NO_OF_INLIERS_1" << min_no_of_inliers_1;
	outputs << "MATCH_THRESHOLD_1" << match_conf_1;

	outputs << "MIN_NO_OF_INLIERS_2" << min_no_of_inliers_2;
	outputs << "MATCH_THRESHOLD_2" << match_conf_2;
	outputs << "ROI_ROWS" << roi_rows;

	outputs << "no_of_input_images" << no_of_inputs;
	outputs << "input_image_width" << full_images[1].cols;
	outputs << "input_image_height" << full_images[1].rows;
	outputs << "total_no_of_stitched_parts" << counter2;
	outputs << "effective_stitched_parts" << effective_parts;
	outputs << "expected_result_image_height" << new_total_height;
	outputs << "total_stitching_time_in_microseconds" << getElapsedTime(&s);
	outputs.release();

	cv::FileStorage other_outputs("matrices.xml", cv::FileStorage::WRITE);
	other_outputs << "FORWARD" << stitchForward;		
	other_outputs << "effective_stitched_parts" << effective_parts;
	other_outputs << "accu_no_of_frames" << new_no_of_frames_accu;
	other_outputs << "accu_no_of_rows" << accu_no_of_rows;
	other_outputs << "rotation_angle" << rotation_angle;
	other_outputs << "scaling" << scaling;
	other_outputs << "new_no_of_rows" << new_no_of_rows;
	other_outputs << "new_no_of_rows_accu" << new_no_of_rows_accu;
	other_outputs << "new_rotation_angle" << new_rotation_angle;
	other_outputs << "new_scaling" << new_scaling;
	other_outputs.release();

	std::cout << "Completed.\n";
	return 0;
}

simpleStitchReturn simpleStitchTransparent(vector<Mat> images, vector<Mat> masks)
{
	simpleStitchReturn res;	
	res.match = true;
	res.original = true;
	res.straight = true;
	res.forward = true;

	// Find features
	Ptr<FeaturesFinder> finder = makePtr<OrbFeaturesFinder>();
	vector<ImageFeatures> features(2);

	vector<Rect> rois_0(1), rois_1(1);
	rois_0[0] = Rect(0, start_0, images[0].cols, roi_rows_0);
	rois_1[0] = Rect(0, 0, images[1].cols, roi_rows_1);

	(*finder)(images[0], features[0], rois_0);
	(*finder)(images[1], features[1], rois_1);

	finder->collectGarbage();

	// Match features
	MatchesInfo pairwise_match;
	Ptr<FeaturesMatcher> matcher;
	matcher = makePtr<AffineBestOf2NearestMatcher>(false, false, match_conf, min_no_of_inliers);

	(*matcher)(features[0], features[1], pairwise_match);
	matcher->collectGarbage();

	res.no_of_inliers = pairwise_match.num_inliers;

	if (res.no_of_inliers >= min_no_of_inliers)
	{		
		// Stitch first image onto the second image
		Mat H12;
		pairwise_match.H.convertTo(H12, CV_32F);

		double tan_here = H12.at<float>(0, 1) / H12.at<float>(0, 0);
		res.rotation_angle = atan(tan_here);
		res.scaling = H12.at<float>(0, 0) / cos(res.rotation_angle);

		if (abs(res.scaling - 1) < scaling_range)
		{
			vector<Point2f> src_vertices = { Point2f(0, 0), Point2f(images[0].cols - 1, 0), Point2f(images[0].cols - 1, images[0].rows - 1), Point2f(0, images[0].rows - 1) };
			vector<Point2f> dst_vertices;

			perspectiveTransform(src_vertices, dst_vertices, H12);		

			if (dst_vertices[0].y < dst_vertices[2].y && dst_vertices[1].y < dst_vertices[3].y)
			{
				double maxTop = max(dst_vertices[0].y, dst_vertices[1].y);

				if (maxTop <= -min_no_of_rows)
				{
					res.no_of_rows = cvRound(-maxTop);
					
					// find size of final image
					double maxCols, maxRows, minCols, minTop;
					minTop = min(dst_vertices[0].y, dst_vertices[1].y);
					maxRows = images[1].rows;

					vector<double> cols(6);
					for (int i = 0;i < 4;i++)
						cols[i] = dst_vertices[i].x;
					cols[4] = 0;
					cols[5] = images[1].cols;

					sort(cols.begin(), cols.end());
					minCols = cols[0];
					maxCols = cols[5];

					// perform a translation after transformation of the first image and its mask
					Mat translation_matrix = (Mat_<float>(3, 3) << 1, 0, -minCols, 0, 1, -minTop, 0, 0, 1);
					Mat_<float> newH12 = translation_matrix * H12;
					Mat transformedImage;
					cv::warpPerspective(images[0], transformedImage, newH12, Size(cvRound(maxCols) - cvRound(minCols), cvRound(maxRows) - cvRound(minTop)));

					Mat transformedMask, finalMask;
					cv::warpPerspective(masks[0], transformedMask, newH12, transformedImage.size());

					// copy both images onto final image and both masks onto final mask
					Mat finalImage(Size(cvRound(maxCols) - cvRound(minCols), cvRound(maxRows) - cvRound(maxTop)), images[1].type());
					Mat transformedImage_roi = transformedImage(Rect(0, cvRound(abs(dst_vertices[0].y - dst_vertices[1].y)), transformedImage.cols, cvRound(-maxTop)));
					transformedImage_roi.copyTo(finalImage(Rect(0, 0, transformedImage_roi.cols, transformedImage_roi.rows)));
					images[1].copyTo(finalImage(Rect(cvRound(-minCols), cvRound(-maxTop), images[1].cols, images[1].rows)));
					res.result = finalImage;

					finalMask.create(finalImage.size(), CV_8U);
					finalMask.setTo(Scalar::all(0));
					Mat transformedMask_roi = transformedMask(Rect(0, cvRound(abs(dst_vertices[0].y - dst_vertices[1].y)), transformedMask.cols, cvRound(-maxTop)));
					transformedMask_roi.copyTo(finalMask(Rect(0, 0, transformedMask_roi.cols, transformedMask_roi.rows)));
					masks[1].copyTo(finalMask(Rect(cvRound(-minCols), cvRound(-maxTop), masks[1].cols, masks[1].rows)));
					res.mask = finalMask;
				}
				else
					res.forward = false;
			}
			else
				res.straight = false;
		}
		else
			res.original = false;
	}
	else
		res.match = false;

	return res;	
}
