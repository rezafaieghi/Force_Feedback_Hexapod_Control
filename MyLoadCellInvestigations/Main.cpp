#include "Config.h"
#include "LoadCellClass.h"

#include <Windows.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <conio.h>
#include <fstream>



void GetLoadCellLoads(LoadCellClass& lc);



int main(void)
{
	LoadCellClass lc = LoadCellClass();

	GetLoadCellLoads(lc);
}



void GetLoadCellLoads(LoadCellClass& lc)
{
	float load_cell_values[DOF] = {};
	float raw_load_cell_values[DOF] = {};
	char style = 'c';
	int duration = 0;
	float time_delay = 0.002f;
	unsigned int marker = 1000;
	std::vector<double> load_vec = { 0, 0, 0, 0, 0, 0 }; //added
	std::vector<std::vector<double>> loads; //added
	std::vector<std::vector<double>> loads1; //added

											 // start load cell if not already started
	if (lc.GetLoads(load_cell_values) != 0)
	{
		fprintf(stdout, "Starting load cell\n\n");
		lc.Initialize(LOAD_CELL_CALIBRATION_PATH.c_str(), LOAD_CELL_TRANSFORMATION, SAMPLE_RATE, LOADCELL_CHANNEL);
		Sleep(2000);
		lc.SetBias();
		Sleep(2000);
	}

	fprintf(stdout, "Continous (c) or finite (f)?\n> ");
	std::cin >> style;
	switch (style)
	{
	case 'c':
		fprintf(stdout, "Time delay between samples (ms)?\n> ");
		std::cin >> time_delay;
		Sleep(2000);
		fprintf(stdout, "Hit ESC key to stop\n");

		while (1)
		{
			if (_kbhit())
			{
				int stop_char = _getch();
				if (stop_char == 27) break;
				if (stop_char == 98) lc.SetBias();
				if (stop_char == 32)
				{
					lc.GetLoads(load_cell_values);
					for (int z = 0; z < 6; z++) load_vec[z] = static_cast<double>(load_cell_values[z]);
					loads1.push_back(load_vec);
				}
			}
			lc.GetLoads(load_cell_values);
			lc.GetRawLoads(raw_load_cell_values);
			printf("%f, %f, %f, %f, %f, %f\n", load_cell_values[0], load_cell_values[1], load_cell_values[2],
				load_cell_values[3], load_cell_values[4], load_cell_values[5]);
			Sleep(static_cast<DWORD>(time_delay));
			for (int z = 0; z < 6; z++) load_vec[z] = static_cast<double>(load_cell_values[z]);
			loads.push_back(load_vec);
		}
		break;
	case 'f':
		fprintf(stdout, "How many to display (s)?\n> ");
		std::cin >> duration;
		fprintf(stdout, "Time delay between samples (ms)?\n> ");
		std::cin >> time_delay;
		for (float i = 0; i < duration; i += time_delay)
		{
			lc.GetLoads(load_cell_values);
			printf("%f, %f, %f, %f, %f, %f\n", load_cell_values[0], load_cell_values[1], load_cell_values[2],
				load_cell_values[3], load_cell_values[4], load_cell_values[5]);
			Sleep(static_cast<DWORD>(time_delay));
		}
		break;
	default:
		fprintf(stdout, "Wrong key %c", style);
		break;
	}

	int size = loads.size();
	std::string save_paths = "drifting.csv";
	std::ofstream savefile(save_paths); //loops through each load file name to separate the loads
	for (int row = 0; row < size; row++) //used to loop through all the vectors 
	{
		for (int column = 0; column <= 5; column++)	//used to loop through each element in the specified variable Fx,Fy,Fz,Mx,My,Mz
		{
			savefile << loads[row][column]; //writes the specified element to the file
			if (column != 5)
			{
				savefile << ", ";																		//writes comma after each value in the file
			}
		}
		savefile << "\n";																				//writes a new line in the file onces a vector is completed
	};
	int size1 = loads1.size();
	std::string save_paths1 = "C:/Users/HMMS/Documents/GitHub/Thesis/KUKA LWR/MHP/loads/selected_loads.csv";
	std::ofstream savefile1(save_paths1); //loops through each load file name to separate the loads
	for (int row = 0; row < size1; row++) //used to loop through all the vectors 
	{
		for (int column = 0; column <= 5; column++) //used to loop through each element in the specified variable Fx,Fy,Fz,Mx,My,Mz
		{
			savefile1 << loads1[row][column]; //writes the specified element to the file
			if (column != 5)
			{
				savefile1 << ", "; //writes comma after each value in the file
			}
		}
		savefile1 << "\n"; //writes a new line in the file onces a vector is completed
	};
}