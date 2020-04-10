#include "Debugger.h"

#include <iostream>
#include <fstream>

namespace vrmotioncompensation
{
	namespace driver
	{
		Debugger::Debugger()
		{

		}

		void Debugger::Start(int MaxDataPoints)
		{
			MaxDebugValues = MaxDataPoints;

			DebugCounter = 0;

			for (int i = 0; i < 20; i++)
			{
				DebugDataV3[i].Data.clear();
			}

			for (int i = 0; i < 5; i++)
			{
				DebugDataQ4[i].Data.clear();
			}

			DebugTiming.clear();

			WroteToFile = false;
			DebuggerRunning = true;
			DebugTimer.start();
		}

		void Debugger::Stop()
		{
			DebuggerRunning = false;
		}

		bool Debugger::IsRunning()
		{
			return DebuggerRunning;
		}

		void Debugger::CountUp()
		{
			if (DebuggerRunning)
			{
				if (DebugCounter > 1)
				{
					for (int i = 0; i < 20; i++)
					{
						if (DebugDataV3[i].InUse)
						{
							if (DebugDataV3[i].Data.size() < DebugCounter)
							{
								DebugDataV3[i].Data.push_back(DebugDataV3[i].Data[DebugDataV3[i].Data.size() - 1]);
							}

							/*if (DebugDataV3[i].Data[DebugCounter].v[0] == 0.0 && DebugDataV3[i].Data[DebugCounter - 1].v[0] != 0.0)
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
							}*/
						}
					}

					for (int i = 0; i < 5; i++)
					{
						if (DebugDataQ4[i].InUse)
						{
							if (DebugDataQ4[i].Data.size() < DebugCounter)
							{
								DebugDataQ4[i].Data.push_back(DebugDataQ4[i].Data[DebugDataQ4[i].Data.size() - 1]);
							}

							/*if (DebugDataQ4[i].Data[DebugCounter].w == 0.0 && DebugDataQ4[i].Data[DebugCounter - 1].w != 0.0)
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
							}*/
						}
					}				
				}

				if (DebugCounter >= MaxDebugValues)
				{
					DebuggerRunning = false;
				}
				else
				{
					DebugCounter++;
				}

				DebugTiming.push_back(DebugTimer.seconds());
			}
		}

		void Debugger::AddDebugData(vr::HmdVector3d_t Data, int ID)
		{
			if (DebuggerRunning)
			{
				DebugDataV3[ID].Data.push_back(Data);
			}			
		}

		void Debugger::AddDebugData(vr::HmdQuaternion_t Data, int ID)
		{
			if (DebuggerRunning)
			{
				DebugDataQ4[ID].Data.push_back(Data);
			}			
		}

		void Debugger::AddDebugData(const double Data[3], int ID)
		{
			if (DebuggerRunning)
			{
				vr::HmdVector3d_t temp = { Data[0], Data[1], Data[2] };
				DebugDataV3[ID].Data.push_back(temp);
			}
		}

		bool Debugger::IsInSync()
		{
			return InSync;
		}

		void Debugger::SetInSync(bool Sync)
		{
			InSync = Sync;
		}

		void Debugger::SetDebugNameQ4(std::string Name, int ID)
		{
			DebugDataQ4[ID].Name = Name;
			DebugDataQ4[ID].InUse = true;
		}

		void Debugger::SetDebugNameV3(std::string Name, int ID)
		{
			DebugDataV3[ID].Name = Name;
			DebugDataV3[ID].InUse = true;
		}

		void Debugger::SetZeroPos(vr::HmdVector3d_t vPos, const double vPosRaw[3], vr::HmdQuaternion_t qPos, vr::HmdQuaternion_t qPosRaw)
		{
			vZeroPos = vPos;
			vZeroPosRaw.v[0] = vPosRaw[0];
			vZeroPosRaw.v[1] = vPosRaw[1];
			vZeroPosRaw.v[2] = vPosRaw[2];
			qZeroPos = qPos;
			qZeroPosRaw = qPosRaw;
		}

		void Debugger::SetLPFValue(double value)
		{
			LPFValue = value;
		}

		void Debugger::WriteFile()
		{
			if (!WroteToFile)
			{		
				int minData = DebugCounter;

				for (int i = 0; i < 5; i++)
				{
					if (DebugDataQ4[i].InUse && DebugDataQ4[i].Data.size() < minData)
					{
						minData = DebugDataQ4[i].Data.size();
					}
				}

				for (int i = 0; i < 20; i++)
				{
					if (DebugDataQ4[i].InUse && DebugDataV3[i].Data.size() < minData)
					{
						minData = DebugDataV3[i].Data.size();
					}
				}

				if (DebugTiming.size() < minData)
				{
					minData = DebugTiming.size();
				}
				
				std::ofstream DebugFile;
				DebugFile.open("MotionData.txt");

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
				for (int i = 0; i < minData; i++)
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
		}
	}
}
