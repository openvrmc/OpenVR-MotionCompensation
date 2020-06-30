#include "Debugger.h"
#include "../logging.h"

#include <iostream>
#include <fstream>

namespace vrmotioncompensation
{
	namespace driver
	{
		Debugger::Debugger()
		{

		}

		Debugger::~Debugger()
		{

		}

		void Debugger::Start()
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			DebugCounter = 0;
			WroteToFile = false;
			DebuggerRunning = true;
			DebugTimer.start();
			LOG(DEBUG) << "Logger started";
		}

		void Debugger::Stop()
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			DebuggerRunning = false;

			LOG(DEBUG) << "Logger stopped";
		}

		bool Debugger::IsRunning()
		{
			return DebuggerRunning;
		}

		void Debugger::CountUp()
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (DebuggerRunning)
			{
				DebugTiming[DebugCounter] = DebugTimer.seconds();

				if (DebugCounter >= MAX_DEBUG_ENTRIES - 1)
				{
					DebuggerRunning = false;
				}
				else
				{
					DebugCounter++;
					Ref = Hmd = false;
				}
			}
		}

		void Debugger::AddDebugData(vr::HmdVector3d_t Data, int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (DebuggerRunning)
			{
				DebugDataV3[ID].Data[DebugCounter] = Data;
			}			
		}

		void Debugger::AddDebugData(vr::HmdQuaternion_t Data, int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (DebuggerRunning)
			{
				DebugDataQ4[ID].Data[DebugCounter] = Data;
			}			
		}

		void Debugger::AddDebugData(const double Data[3], int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (DebuggerRunning)
			{
				DebugDataV3[ID].Data[DebugCounter].v[0] = Data[0];
				DebugDataV3[ID].Data[DebugCounter].v[1] = Data[1];
				DebugDataV3[ID].Data[DebugCounter].v[2] = Data[2];
			}
		}

		void Debugger::gotRef()
		{
			Ref = true;
		}

		void Debugger::gotHmd()
		{
			Hmd = true;
		}

		bool Debugger::hasRef()
		{
			return Ref;
		}

		bool Debugger::hasHmd()
		{
			return Hmd;
		}


		void Debugger::SetDebugNameQ4(std::string Name, int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			DebugDataQ4[ID].Name = Name;
			DebugDataQ4[ID].InUse = true;
		}

		void Debugger::SetDebugNameV3(std::string Name, int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			DebugDataV3[ID].Name = Name;
			DebugDataV3[ID].InUse = true;
		}

		void Debugger::WriteFile()
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (!WroteToFile && DebugCounter > 0)
			{
				LOG(DEBUG) << "Trying to write debug file...";

				std::ofstream DebugFile;
				DebugFile.open("MotionData.txt");

				if (!DebugFile.bad())
				{
					unsigned int index = 1;

					LOG(DEBUG) << "Writing " << DebugCounter << " debug points of data";

					//Write title
					DebugFile << "Time;";

					//Quaternions
					for (int i = 0; i < MAX_DEBUG_QUATERNIONS; i++)
					{
						if (DebugDataQ4[i].InUse)
						{
							DebugFile << DebugDataQ4[i].Name << "[" << index << ":4];";
							index += 4;
						}
					}

					//Vectors
					for (int i = 0; i < MAX_DEBUG_VECTORS; i++)
					{
						if (DebugDataV3[i].InUse)
						{
							DebugFile << DebugDataV3[i].Name << "[" << index << ":3];";
							index += 3;
						}
					}
					DebugFile << std::endl;

					//Write data
					for (int i = 0; i < DebugCounter; i++)
					{
						DebugFile << DebugTiming[i] << ";";
				
						for (int j = 0; j < MAX_DEBUG_QUATERNIONS; j++)
						{
							if (DebugDataQ4[j].InUse)
							{
								DebugFile << DebugDataQ4[j].Data[i].w << ";";
								DebugFile << DebugDataQ4[j].Data[i].x << ";";
								DebugFile << DebugDataQ4[j].Data[i].y << ";";
								DebugFile << DebugDataQ4[j].Data[i].z << ";";
							}
						}
				
						for (int j = 0; j < MAX_DEBUG_VECTORS; j++)
						{
							if (DebugDataV3[j].InUse)
							{
								DebugFile << DebugDataV3[j].Data[i].v[0] << ";";
								DebugFile << DebugDataV3[j].Data[i].v[1] << ";";
								DebugFile << DebugDataV3[j].Data[i].v[2] << ";";
							}
						}

						DebugFile << std::endl;
					}

					DebugFile.close();

					WroteToFile = true;
				}
				else
				{
					LOG(ERROR) << "Could not write debug log";
				}
			}
		}
	}
}