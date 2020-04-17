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
				if (DebugCounter > 1)
				{
					for (int i = 0; i < 20; i++)
					{
						if (DebugDataV3[i].InUse)
						{
							if (DebugDataV3[i].Data[DebugCounter].v[0] == 0.0 && DebugDataV3[i].Data[DebugCounter - 1].v[0] != 0.0)
							{
								DebugDataV3[i].Data[DebugCounter].v[0] = DebugDataV3[i].Data[DebugCounter - 1].v[0];
							}

							if (DebugDataV3[i].Data[DebugCounter].v[1] == 0.0 && DebugDataV3[i].Data[DebugCounter - 1].v[1] != 0.0)
							{
								DebugDataV3[i].Data[DebugCounter].v[1] = DebugDataV3[i].Data[DebugCounter - 1].v[1];
							}

							if (DebugDataV3[i].Data[DebugCounter].v[2] == 0.0 && DebugDataV3[i].Data[DebugCounter - 1].v[2] != 0.0)
							{
								DebugDataV3[i].Data[DebugCounter].v[2] = DebugDataV3[i].Data[DebugCounter - 1].v[2];
							}
						}
					}

					for (int i = 0; i < 5; i++)
					{
						if (DebugDataQ4[i].InUse)
						{
							if (DebugDataQ4[i].Data[DebugCounter].w == 0.0 && DebugDataQ4[i].Data[DebugCounter - 1].w != 0.0)
							{
								DebugDataQ4[i].Data[DebugCounter].w = DebugDataQ4[i].Data[DebugCounter - 1].w;
							}

							if (DebugDataQ4[i].Data[DebugCounter].x == 0.0 && DebugDataQ4[i].Data[DebugCounter - 1].x != 0.0)
							{
								DebugDataQ4[i].Data[DebugCounter].x = DebugDataQ4[i].Data[DebugCounter - 1].x;
							}

							if (DebugDataQ4[i].Data[DebugCounter].y == 0.0 && DebugDataQ4[i].Data[DebugCounter - 1].y != 0.0)
							{
								DebugDataQ4[i].Data[DebugCounter].y = DebugDataQ4[i].Data[DebugCounter - 1].y;
							}

							if (DebugDataQ4[i].Data[DebugCounter].z == 0.0 && DebugDataQ4[i].Data[DebugCounter - 1].z != 0.0)
							{
								DebugDataQ4[i].Data[DebugCounter].z = DebugDataQ4[i].Data[DebugCounter - 1].z;
							}
						}
					}				
				}

				DebugTiming[DebugCounter] = DebugTimer.seconds();

				if (DebugCounter >= 10000)
				{
					DebuggerRunning = false;
				}
				else
				{
					DebugCounter++;
				}
			}
		}

		void Debugger::AddDebugData(vr::HmdVector3d_t Data, int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (DebuggerRunning)
			{
				DebugDataV3[ID].Data[DebugCounter].v[0] = Data.v[0];
				DebugDataV3[ID].Data[DebugCounter].v[1] = Data.v[1];
				DebugDataV3[ID].Data[DebugCounter].v[2] = Data.v[2];
			}			
		}

		void Debugger::AddDebugData(vr::HmdQuaternion_t Data, int ID)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (DebuggerRunning)
			{
				DebugDataQ4[ID].Data[DebugCounter].w = Data.w;
				DebugDataQ4[ID].Data[DebugCounter].x = Data.x;
				DebugDataQ4[ID].Data[DebugCounter].y = Data.y;
				DebugDataQ4[ID].Data[DebugCounter].z = Data.z;
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

		bool Debugger::IsInSync()
		{
			return InSync;
		}

		void Debugger::SetInSync(bool Sync)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			InSync = Sync;
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

		void Debugger::SetZeroPos(vr::HmdVector3d_t vPos, const double vPosRaw[3], vr::HmdQuaternion_t qPos, vr::HmdQuaternion_t qPosRaw)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			vZeroPos = vPos;
			vZeroPosRaw.v[0] = vPosRaw[0];
			vZeroPosRaw.v[1] = vPosRaw[1];
			vZeroPosRaw.v[2] = vPosRaw[2];
			qZeroPos = qPos;
			qZeroPosRaw = qPosRaw;
		}

		void Debugger::SetLPFValue(double value)
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			LPFValue = value;
		}

		void Debugger::WriteFile()
		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mut);

			if (!WroteToFile)
			{
				LOG(DEBUG) << "Trying to write debug file...";

				std::ofstream DebugFile;
				DebugFile.open("MotionData.txt");

				if (!DebugFile.bad())
				{
					LOG(DEBUG) << "Writing " << DebugCounter << " debug points of data";

					//Write ZeroPos and filter setting
					DebugFile << "ZeroPos X;" << vZeroPos.v[0] << ";Y;" << vZeroPos.v[1] << ";Z;" << vZeroPos.v[2] << ";";
					DebugFile << "ZeroPosRaw X;" << vZeroPosRaw.v[0] << ";Y;" << vZeroPosRaw.v[1] << ";Z;" << vZeroPosRaw.v[2] << ";";
					DebugFile << "ZeroPos w;" << qZeroPos.w << ";x;" << qZeroPos.x << ";y;" << qZeroPos.y << ";t;" << qZeroPos.z << ";";
					DebugFile << "ZeroPosRaw w;" << qZeroPosRaw.w << ";x;" << qZeroPosRaw.x << ";y;" << qZeroPosRaw.y << ";z;" << qZeroPosRaw.z << ";";
					DebugFile << "LPF Beta;" << LPFValue << ";";
					DebugFile << std::endl;

					//Write title
					DebugFile << "Time;";

					//Quaternions
					for (int i = 0; i < 5; i++)
					{
						if (DebugDataQ4[i].InUse)
						{
							DebugFile << DebugDataQ4[i].Name << " w;" << "x;" << "y;" << "z;";
						}
					}

					//Vectors
					for (int i = 0; i < 20; i++)
					{
						if (DebugDataV3[i].InUse)
						{
							DebugFile << DebugDataV3[i].Name << " X;" << "Y;" << "Z;";
						}
					}
					DebugFile << std::endl;

					//Write data
					for (int i = 0; i < DebugCounter; i++)
					{
						DebugFile << DebugTiming[i] << ";";
				
						for (int j = 0; j < 5; j++)
						{
							if (DebugDataQ4[j].InUse)
							{
								DebugFile << DebugDataQ4[j].Data[i].w << ";";
								DebugFile << DebugDataQ4[j].Data[i].x << ";";
								DebugFile << DebugDataQ4[j].Data[i].y << ";";
								DebugFile << DebugDataQ4[j].Data[i].z << ";";
							}
						}
				
						for (int j = 0; j < 20; j++)
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