#include <oaidl.h>
#include <atlsafe.h>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#define _USE_MATH_DEFINES		// Get definition of M_PI (value of Pi)
#include <math.h>
#include <list>
#include <sstream>
#include <cstring>
#include <chrono>
#include <conio.h>

#import "D:\Google Drive\UWO\Postdoc\Hexapod API\HexapodSDK\bin\x64\HexapodCOM.dll"
using namespace std;
using std::cin;
using std::cout;
using std::string;
using std::to_string;
using std::transform;
using std::vector;
using namespace std::chrono;
using namespace HexapodCOMLib;					// Namespace of the MHP

///////////////////////////////////////////////////////////////////////////////
////                         Function Declarations                         ////
///////////////////////////////////////////////////////////////////////////////

//~~~~~~~~~~ Loop Functions ~~~~~~~~~~//
void MHPOpen(IHexapodPtr& mhp);
void MHPZero(IHexapodPtr& mhp);
void MHPcnc(IHexapodPtr& mhp);

//~~~~~~~~~~ Additional Call Functions ~~~~~~~~~~//
vector<double> VariantToVector(const _variant_t& var);
_variant_t VectorToVariant(vector<double>& vec);
void MHPPrintCurrentPos(IHexapodPtr& mhp);
vector<double> MHPTranspose(vector<double>& matrix);
vector<double> DoubleVariantToVector(const _variant_t& var);
void MHPVelocity(IHexapodPtr& mhp);

///////////////////////////////////////////////////////////////////////////////
////                           ENUMS & MAPPINGS                            ////
///////////////////////////////////////////////////////////////////////////////
enum Programs
{
	MHPOPEN,
	MHPZERO,
	MHPCNC
};

static map<Programs, string> programs =
{
	{ MHPOPEN, "MHP Open" },
	{ MHPZERO, "MHP Zero" },
	{ MHPCNC, "MHP Move Through Path" },
};

int main()
{
	IHexapodPtr mhp;																//MHP variable that communicates to the robot
	HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	hr = mhp.CreateInstance(__uuidof(HexapodCOMLib::Hexapod));						//Needed to make the MHP object

	int selection = -1;
	while (true)
	{
		cout << "\n|==================== MAIN MENU ====================|\n";
		fprintf(stdout, "|                  SELECT A PROGRAM\n");
		for (auto const &it : programs) {
			printf("|        (%d) %s\n", it.first, it.second.c_str());
		}
		cout << "|===================================================|\n";
		cin >> selection;
		switch (selection)
		{
		case MHPOPEN:
			MHPOpen(mhp);
			break;
		case MHPZERO:
			MHPZero(mhp);
			break;
		case MHPCNC:
			MHPcnc(mhp);
			break;
		default:
			fprintf(stdout, "Incorrect option selected\n");
			cin.clear();
			cin.ignore(10000, '\n');
			break;
		}
		selection = -1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
////                            Loop Functions                             ////
///////////////////////////////////////////////////////////////////////////////

//~~~~~~~~~~ MHP OPEN ~~~~~~~~~~//
void MHPOpen(IHexapodPtr& mhp)
{
	cout << "\nWhich robot would you like to using?\n"
		<< "1)Small Robot (MHP11_SN11007)\n"
		<< "2)White Robot (MHP-11L_SN11014)\n"
		<< "3)CT Robot (MHP17L_SN17011_HMMS)\n"
		<< "4)Exit Selection\n\n";

	_bstr_t device_address;
	_bstr_t serial_number;
	int robot;

	while (1)
	{
		cin >> robot;
		switch (robot)
		{
		case 1:
			device_address = L"MHP11_SN11007";
			serial_number = L"11007";
			break;
		case 2:
			device_address = L"MHP-11L_SN11014";
			serial_number = L"11014";
			break;
		case 3:
			device_address = L"MHP17L_SN17011_HMMS";
			serial_number = L"17011";
			break;
		case 4:
			break;
		default:
			cout << "\nImproper selection, try again\n"
				<< "\nWhich robot would you like to using?\n"
				<< "1)Small Robot (MHP11_SN11007)\n"
				<< "2)White Robot (MHP-11L_SN11014)\n"
				<< "3)CT Robot (MHP17L_SN17011_HMMS)\n"
				<< "4)Exit Selection\n\n";
		}
		break;
	}

	try
	{
		mhp->Open(device_address);					//Opens the linked platform file
		mhp->Connect(VARIANT_FALSE, L"", serial_number);				//(VARIANT_"" (TRUE = simulate platform, FALSE = real platform), "address", "serial #")
		cout << "MHP Connected!\n";
	}
	catch (const _com_error& ex)
	{
		cout << ex.Description();
		cout << "Could not Open/Connect\nTurn On Power!\n\n";
	}
	try
	{
		mhp->Home();											//Homes the MHP
		while (mhp->Moving)										//Loop to prevent any commands until robot is done moving
		{
		}
		std::cout << "MHP Homed!\n\n";
	}
	catch (const _com_error& ex)
	{
		std::cout << ex.Description();
		std::cout << "\nCould not Home!\n\n";
	}											//Homes the MHP
}
//~~~~~~~~~~ MHP ZERO ~~~~~~~~~~//
void MHPZero(IHexapodPtr& mhp)
{
	try
	{
		mhp->MovePosition(VARIANT_TRUE, L"Zero");				//Zeros MHP
		while (mhp->Moving)										//Loop to prevent any commands until robot is done moving
		{
		}
		MHPPrintCurrentPos(mhp);
		std::cout << "MHP Zeroed!\n\n";
	}
	catch (const _com_error& ex)
	{
		std::cout << "\nCould not Zero\nMake Sure MHP Is Turned On and Has Been Connected!\n\n";
	}

}
//~~~~~~~~~~ MHP CNC ~~~~~~~~~~//
void MHPcnc(IHexapodPtr& mhp)
{
	try
	{
		int length = 0;																							//stores the length (number of positions from the file given)

		//===== Connect and Zero MHP =====//
		char mhp1 = 'n';
		cout << "\nWould you like to connect Robot Platform (y) or (n)?\n";
		cin >> mhp1;
		cout << endl;
		if (mhp1 == 'y')
		{
			MHPOpen(mhp);
		}
		MHPZero(mhp);																							//moves mhp to zero position

		//===== Set Velocity =====//
		MHPVelocity(mhp);

		//===== Variables =====//
		char zero = 'n';
		int cycle = 1;																							//variable keeps track of which loop the MHP is on
		int cancel = 0;																							//variable to cancel moving loop if necessary
		string filestring;																						//user input path file name
		string savestring;																						//user input save file name
		string anglestring;																						//user input angles file name
		int loops = 0;																							//user input number of loops
		int load_loops = 20;																					//value that holds number of measurements are taken at each point
		cout << "\nWhat path would you like to follow?\n";
		cin >> filestring;
		string full_path = "C:/Users/HMMS/Documents/GitHub/Thesis/KUKA LWR/MHP/paths/" + filestring + ".csv";	//full path name of where the path file should be
		list<vector<double>> position;																			//declares a list of vectors to store all the position data
		vector<double> posvec_flip;																				//declares a vector that can store the value of a single position move
		vector<double> posvec_norm;																				//declares a vector that can store the value of a single position move
		vector<double> intermediate = { 0, 0, 0, 0, 0, 0 };
		vector<vector<double>> intermediate_loads;
		int point_count = 1;

		ifstream pathfile(full_path);																		//path file that is used for below code
		if (pathfile.fail())
		{
			cout << "\nFile Name Does Not Exist\n\n";
			return;
		}
		cout << "================================================\n";
		cout << "Now on Loop: " << cycle << " of " << loops << "\n";
		string line;

		int i = 0;																							//variable to control looping through the 16 values of each line
		while (getline(pathfile, line) && cancel == 0)														//gets a line from the csv as a string ex: "1,0,0,0,0,1,0,0,0,0,1,0,10,0,0,1"
		{
			stringstream ss(line);																			//
			while (ss.good() && cancel == 0)																//
			{
				string substr;																				//string variable to break apart the line of data, based on comma that separates each value
				getline(ss, substr, ',');																	//uses the line data gathered, and breaks apart each value
				double h = stod(substr);																	//converts string values to useable double values
				posvec_norm.push_back(h);																	//adds the value to the end of a vector
				i++;																						//increments a counting variable
				if (i == 16 && cancel == 0)
				{
					position.push_back(posvec_norm);														//adds the vector to the end of the list (once the vector has all 16 values)
					posvec_flip = MHPTranspose(posvec_norm);
					cout << "Moving to: " << posvec_flip[12] << ", " << posvec_flip[13] << ", " << posvec_flip[14] << "\n";
					cout << "Now on Point: " << point_count << "\n";
					variant_t v = VectorToVariant(posvec_flip);												//converts vector to useable variable to move platform
					mhp->MoveMatrix(VARIANT_TRUE, L"Reamer", L"Reamer Zero", v);							//uses v value to move the platform relative to the base by 1 mm in x
					while (mhp->Moving && cancel == 0)
					{
						if (_kbhit())
						{
							int stop_char = _getch();
							if (stop_char == 27)
							{
								mhp->Stop();
								cancel = 1;
								break;
							}																				//this two if statements check if escape is pressed, if it is it will stop the robot, basically and estop
						}																					//escape button press command
					}
					//delay(125);																				//delay used to allow the load cell and MHP to settle reduce inertial effects in (milliseconds) 500
					posvec_norm.clear();																	//clears the position vector so it can store the next line on the next loop
					MHPPrintCurrentPos(mhp);																//function that prints a matrix
					i = 0;																					//counter
				}
			}
			point_count++;
		}
	}
	catch (const _com_error& ex)
	{
		std::cout << "Error! Could Not Run Path!\nMake Sure MHP Is Turned On and Has Been Connected!\n\n";
	}
}

///////////////////////////////////////////////////////////////////////////////
////                       Additional Call Function                        ////
///////////////////////////////////////////////////////////////////////////////

//~~~~~~~~~~ Variant to Vector ~~~~~~~~~~//
vector<double> VariantToVector(const _variant_t& var)
{
	CComSafeArray<double> SafeArray;
	SafeArray.Attach(var.parray);
	size_t count = SafeArray.GetCount();
	std::vector<double> vec(count);
	for (size_t i = 0; i < count; i++)
	{
		vec[i] = SafeArray.GetAt((ULONG)i);
	}
	SafeArray.Detach();
	return vec;
}
//~~~~~~~~~~ Vector to Variant ~~~~~~~~~~//
_variant_t VectorToVariant(vector<double>& vec)
{
	size_t count = vec.size();
	CComSafeArray<double> sa((ULONG)count);
	for (size_t i = 0; i < count; i++)
	{
		sa.SetAt((LONG)i, vec[i]);
	}
	_variant_t var;
	CComVariant(sa).Detach(&var);
	return var;
}
//~~~~~~~~~~ Print Current Pose ~~~~~~~~~~//
void MHPPrintCurrentPos(IHexapodPtr& mhp)
{
	variant_t pos = mhp->GetPositionMatrix(PositionActual, L"Reamer", L"Reamer Zero");	//gets position of platform with respect to platform zero
	vector<double> pos_ = DoubleVariantToVector(pos);										//converts the get pos variant to vector, after the variant is converted from a array
	vector<double> current_pos_transpose = MHPTranspose(pos_);								//Transposes the matrix so it is in the conventional form
	cout << "Current Position: \n";
	for (int i = 0; i < 16; i++)
	{
		std::cout << std::setprecision(4) << std::fixed;										//sets number of decimal points of numbers being displayed
		if (current_pos_transpose[i] >= 0)
		{
			cout << " ";																	//adds a space if number doesn't have negative sign in order to keep everything in line
		}
		cout << current_pos_transpose[i] << ", ";
		if (i == 3 || i == 7 || i == 11 || i == 15)											//after 4 terms prints to line to get the 4x4 matrix grid
		{
			cout << "\n";
		}
	}
	cout << "\n";
	std::cout << std::setprecision(4) << std::defaultfloat;									//sets number of decimal points of numbers being displayed
}
//~~~~~~~~~~ Transpose Matrix MHP ~~~~~~~~~~//
vector<double> MHPTranspose(vector<double>& matrix)
{
	vector<double> matrix_transpose = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	//		Matrix						  Transpose
	//  0,  1,  2,  3,					0,  4,  8, 12,
	//  4,  5,  6,  7,		--->		1,  5,  9, 13,
	//  8,  9, 10, 11,					2,  6, 10, 14,
	// 12, 13, 14, 15,				    3,  7, 11, 15,
	matrix_transpose[0] = matrix[0];
	matrix_transpose[1] = matrix[4];
	matrix_transpose[2] = matrix[8];
	matrix_transpose[3] = matrix[12];
	matrix_transpose[4] = matrix[1];
	matrix_transpose[5] = matrix[5];
	matrix_transpose[6] = matrix[9];
	matrix_transpose[7] = matrix[13];
	matrix_transpose[8] = matrix[2];
	matrix_transpose[9] = matrix[6];
	matrix_transpose[10] = matrix[10];
	matrix_transpose[11] = matrix[14];
	matrix_transpose[12] = matrix[3];
	matrix_transpose[13] = matrix[7];
	matrix_transpose[14] = matrix[11];
	matrix_transpose[15] = matrix[15];

	return matrix_transpose;
}
//~~~~~~~~~~ Double Variant to Vector ~~~~~~~~~~//
vector<double> DoubleVariantToVector(const _variant_t& var)
{
	CComSafeArray<double> SafeArray;
	SafeArray.Attach(var.parray);
	double cElement;
	LONG aIndex[2];
	int count = 0;
	std::vector<double> vec(16);
	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			aIndex[0] = x;
			aIndex[1] = y;
			SafeArray.MultiDimGetAt(aIndex, cElement); //HRESULT hr = 
			//ATLASSERT(hr == S_OK);
			//ATLASSERT(cElement == vec[count]);
			vec[count] = cElement;
			count++;
		}
	}
	SafeArray.Detach();
	return vec;
}
//~~~~~~~~~~ MHP Set Velocity ~~~~~~~~~~//
void MHPVelocity(IHexapodPtr& mhp)
{
	double velocity = 1;
	cout << "\nSet relative velocity between 0.00 - 1.00 (default = 1)\n";
	cin >> velocity;
	mhp->put_Velocity(velocity);
}