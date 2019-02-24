#pragma once

#include <string>

const unsigned int DOF = 6;
const unsigned int SAMPLES_PER_CHANNEL = 10;
const unsigned int SAMPLES_TILL_EVENT = 10;
const unsigned int MAX_VOLTAGE_VALUE = 10;
const signed int MIN_VOLTAGE_VALUE = -10;
const float DAQ_TIMEOUT = 10.0f;
const unsigned int FILTER_ORDER_NUMBER = 3;
const unsigned int SUCCESS = 0;
const unsigned int ERROR_CALIBRATION = 1;
const unsigned int ERROR_DAQ_START = 2;
const unsigned int ERROR_TRANSFORMATION = 3;
const unsigned int ERROR_DAQ_NOT_STARTED = 4;
const unsigned int ERROR_BIAS_NOT_SET = 5;



const float LOAD_CELL_TRANSFORMATION[DOF] = { 37.211f, 0.0f, 154.252f, 0.0f, 0.0f, 0.0f };
const float SAMPLE_RATE = 5000; //in Hz
const std::string LOADCELL_CHANNEL = "Dev1/ai0:5";



const std::string CALIBRATE_STR = "Calibrating load cell";
const std::string TRANSFORM_STR = "Transforming load cell";
const std::string START_STR = "Starting load cell";
const std::string BIAS_STR = "Biasing load cell";
const std::string STOP_STR = "Stopping load cell";
const std::string LOAD_CELL_CALIBRATION_PATH = "FT13865.cal";
