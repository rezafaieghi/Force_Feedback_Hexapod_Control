#include "LoadCellClass.h"

#include <fstream>

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle,
	int32 everyNsamplesEventType, uInt32 nSamples, void *callback_package) {
	int32 error = 0;
	char errBuff[2048] = { '\0' };
	int32 read = 0;

	// unpackage callback package
	CallbackPackage* package = static_cast<CallbackPackage*>(callback_package);

	DAQmxErrChk(DAQmxReadAnalogF64(
		taskHandle,                           // task
		SAMPLES_PER_CHANNEL,                  // samples per channel
		DAQ_TIMEOUT,                          // timeout
		DAQmx_Val_GroupByChannel,             // fill mode
		package->buffer,                      // array to read samples into
		SAMPLES_PER_CHANNEL * DOF,            // size of array
		&read,                                // samples per channel read
		NULL));                               // reserved

												// Voltage array's samples are grouped by channel.
												// e.g.
												//   10 SAMPLES_PER_CHANNEL grouped by channel
												//   transducer_0    transducer_1   ...  transducer_6
												//   [0, 1 ..., 9,   10, 11 ..., 19 ...  50, 51 ..., 59]    
	if (read > 0) {
		// get most recent values and put into raw voltages
		for (unsigned int i = 0; i < DOF; ++i) {
			package->raw_voltages[i] = package->buffer[(SAMPLES_PER_CHANNEL*(i + 1)) - 1];
		}

		//// filtering
		//if (package->filtering_enabled) {
		//	// first pass
		//	package->filter->process(SAMPLES_PER_CHANNEL,
		//		make_2d<SAMPLES_PER_CHANNEL, SAMPLES_PER_CHANNEL*DOF>(package->buffer).data());

		//	// reverse array
		//	for (unsigned int i = 0; i < DOF; ++i)
		//	{
		//		std::reverse(package->buffer + (SAMPLES_PER_CHANNEL * i),
		//			package->buffer + (SAMPLES_PER_CHANNEL*(i + 1) - 1));
		//	}

		//	// second pass
		//	package->filter->process(SAMPLES_PER_CHANNEL,
		//		make_2d<SAMPLES_PER_CHANNEL, SAMPLES_PER_CHANNEL*DOF>(package->buffer).data());
		//}

		// average
		if (package->channel_averaging_enabled == true) {
			double sum = 0;
			for (unsigned int i = 0; i < DOF; ++i) {
				sum = 0;
				for (unsigned int j = 0; j < SAMPLES_PER_CHANNEL; ++j) {
					sum += package->buffer[(i*SAMPLES_PER_CHANNEL) + j];
				}
				package->voltages[i] = static_cast<float>(sum / SAMPLES_PER_CHANNEL);
			}
		}
		else {
			for (unsigned int i = 0; i < DOF; ++i) {
				package->voltages[i] = static_cast<float>(package->buffer[i * 10]);
			}
		}
	} // done read

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}



LoadCellClass::LoadCellClass()
	: task_handle_(0)
	, voltages_()
	, raw_voltages_()
	, bias_()
	, cal_(NULL)
	, lower_threshold_set_flag(true)
	, bias_set_flag(true) {}



LoadCellClass::~LoadCellClass()
{
	// ATI calibration
	if (cal_ != NULL) destroyCalibration(cal_);

	// DAQmx tasks
	if (task_handle_ != 0)
	{
		DAQmxStopTask(task_handle_);
		DAQmxClearTask(task_handle_);
	}
}



int LoadCellClass::Initialize(std::string calibration_path,
	const float* transform,
	const float sample_rate,
	std::string channel,
	bool set_filter)
{
	unsigned int err_val = SUCCESS;

	if (calibration_path != "")
	{
		err_val = SetCalibration(calibration_path.c_str());
		if (err_val != SUCCESS) return ERROR_CALIBRATION;
	}

	if (transform != NULL)
	{
		err_val = SetTransformation(transform);
		if (err_val != SUCCESS) return ERROR_TRANSFORMATION;
	}

	if (sample_rate != 0 && channel != "")
	{
		err_val = StartDAQmx(sample_rate, channel.c_str());
		if (err_val != SUCCESS) return ERROR_DAQ_START;
	}

	if (set_filter) SetFilter();

	return SUCCESS;
}



int LoadCellClass::StartDAQmx(const float sample_rate, const std::string channel) {
	int32 error = 0;
	char errBuff[2048] = { '\0' };
	DBGPRINT("%s", START_STR);

	// package callback data
	callback_.buffer = buffer_;
	callback_.filtering_enabled = filter_enabled_;
	//callback_.filter = &filter_;
	callback_.voltages = voltages_;
	callback_.raw_voltages = raw_voltages_;
	callback_.channel_averaging_enabled = true;

	// configure DAQmx
	DAQmxErrChk(DAQmxCreateTask("", &task_handle_));
	DAQmxErrChk(DAQmxCreateAIVoltageChan(
		task_handle_,                     // task
		channel.c_str(),                  // physical channel
		"",                               // assigned channel name
		DAQmx_Val_Diff,                   // input terminal configuration
		MIN_VOLTAGE_VALUE,                // min val
		MAX_VOLTAGE_VALUE,                // max val
		DAQmx_Val_Volts,                  // units
		NULL));                           // custom scale name
	DAQmxErrChk(DAQmxCfgSampClkTiming(
		task_handle_,                     // task
		"",                               // use internal clock
		sample_rate,                      // sampling rate (Hz)
		DAQmx_Val_Rising,                 // active edge
		DAQmx_Val_ContSamps,              // sample continuously
		SAMPLES_PER_CHANNEL));            // samples per channel
	DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(
		task_handle_,                     // task
		DAQmx_Val_Acquired_Into_Buffer,   // type of event to receive
		SAMPLES_TILL_EVENT,               // number of samples until event
		0,                                // options
		EveryNCallback,                   // callback function
		&callback_));                     // callback data (struct)

	DAQmxErrChk(DAQmxStartTask(task_handle_));
	sample_rate_ = sample_rate;

	return SUCCESS;

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	if (task_handle_ != 0) {
		DAQmxStopTask(task_handle_);
		DAQmxClearTask(task_handle_);
	}
	if (DAQmxFailed(error)) {
		printf("DAQmx Error: %s\n", errBuff);
	}
	return ERROR_DAQ_START;
}



void LoadCellClass::StopDAQmx()
{
	DBGPRINT("%s", STOP_STR);
	if (task_handle_ != 0)
	{
		DAQmxStopTask(task_handle_);
		DAQmxClearTask(task_handle_);
	}
	task_handle_ = 0;
	sample_rate_ = 0;
}



void LoadCellClass::SetFilter(unsigned int order_number, unsigned int cutoff_freq, const bool enable_filter)
{
	//filter_.setup(FILTER_ORDER_NUMBER, sample_rate_, cutoff_freq);
	filter_enabled_ = enable_filter;
}



void LoadCellClass::EnableFilter(bool status) {
	filter_enabled_ = status;
}



int LoadCellClass::SetCalibration(const std::string calibration_file_path)
{
	DBGPRINT("%s", CALIBRATE_STR);
	cal_ = createCalibration(const_cast<char *>(calibration_file_path.c_str()), 1);
	if (cal_ == NULL)
	{
		printf("ERROR: Specified calibration could not be loaded\n");
		return ERROR_CALIBRATION;
	}
	SetForceUnits(cal_, "N");
	SetTorqueUnits(cal_, "N-m");
	return SUCCESS;
}



int LoadCellClass::GetVoltages(float *voltages) const
{
	if (task_handle_ == 0)
	{
		DBGPRINT("Load cell not started\n");
		return ERROR_DAQ_NOT_STARTED;
	}
	else
	{
		for (int i = 0; i < DOF; i++) voltages[i] = voltages_[i];
	}
	return SUCCESS;
}




int LoadCellClass::GetRawVoltages(float* voltages) const
{
	if (task_handle_ == 0)
	{
		DBGPRINT("Load cell not started\n");
		return ERROR_DAQ_NOT_STARTED;
	}
	else
	{
		for (int i = 0; i < DOF; i++) voltages[i] = raw_voltages_[i];
	}
	return SUCCESS;
}



int LoadCellClass::GetLoads(float *forces)
{
	if (task_handle_ == 0)
	{
		DBGPRINT("Load cell not started\n");
		return ERROR_DAQ_NOT_STARTED;
	}
	else
	{
		ConvertToFT(cal_, voltages_, forces);
	}
	return SUCCESS;
}



int LoadCellClass::GetRawLoads(float *forces)
{
	if (task_handle_ == 0)
	{
		DBGPRINT("Load cell not started\n");
		return ERROR_DAQ_NOT_STARTED;
	}
	else
	{
		ConvertToFT(cal_, raw_voltages_, forces);
	}
	return SUCCESS;
}



int LoadCellClass::SetTransformation(const float* transform)
{
	DBGPRINT("%s", TRANSFORM_STR);
	unsigned int err_val = SetToolTransform(cal_,
		const_cast<float *>(transform), "mm", "degrees");
	if (err_val != SUCCESS)
	{
		return ERROR_TRANSFORMATION;
	}
	return SUCCESS;
}



int LoadCellClass::SetBias()
{
	if (task_handle_ == 0) {
		DBGPRINT("Load cell not started\n");
		return ERROR_DAQ_NOT_STARTED;
	}
	else
	{
		ConvertToFT(cal_, raw_voltages_, bias_);  // sets bias_
		Bias(cal_, raw_voltages_);
		DBGPRINT("Set bias: %.3f, %.3f, %.3f, %.3f, %.3f, %.3f\n",
			bias_[0], bias_[1], bias_[2], bias_[3], bias_[4], bias_[5]);
	}
	return SUCCESS;
}



int LoadCellClass::GetBias(float* bias) const
{
	if (!bias_set_flag)
	{
		return ERROR_BIAS_NOT_SET;
	}
	for (int i = 0; i < DOF; ++i) bias[i] = bias_[i];
	return SUCCESS;
}



void LoadCellClass::SetLowerForceThreshold(const float* lower_threshold)
{
	lower_threshold_set_flag = true;
	for (int i = 0; i < DOF; ++i) lower_threshold_[i] = lower_threshold[i];
}



bool LoadCellClass::ExceedsLowerForceThreshold()
{
	if (!lower_threshold_set_flag)
	{
		fprintf(stderr, "Lower threshold has not been set\n");
	}
	if (task_handle_ == 0)
	{
		fprintf(stderr, "Load cell not started\n");
	}
	else
	{
		float loads[DOF];
		GetLoads(loads);
		for (int i = 0; i < DOF; ++i)
		{
			if (abs(loads[i]) > abs(lower_threshold_[i])) return true;
		}
	}
	return false;
}



bool LoadCellClass::IsActive()
{
	return task_handle_ != 0;
}



void LoadCellClass::GetSamplingRate(float& rate) const
{
	rate = sample_rate_;
}