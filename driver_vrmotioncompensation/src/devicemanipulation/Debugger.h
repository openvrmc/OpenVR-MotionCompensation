#pragma once

#include <openvr_driver.h>

#include <boost/timer/timer.hpp>
#include <boost/chrono/chrono.hpp>
#include <chrono\system_clocks.hpp>

namespace vrmotioncompensation
{
	namespace driver
	{
		class Debugger
		{
		private:
			struct DebugDataV3_t
			{
				std::string Name;
				std::vector<vr::HmdVector3d_t> Data;
				bool InUse;

				DebugDataV3_t()
				{
					Name = "";
					InUse = false;
				}
			};

			struct DebugDataQ4_t
			{
				std::string Name;
				std::vector<vr::HmdQuaternion_t> Data;
				bool InUse;

				DebugDataQ4_t()
				{
					Name = "";
					InUse = false;
				}
			};

			template< class Clock > class timer
			{
				typename Clock::time_point tstart;
			public:
				timer()
				{
				}
				typename Clock::duration elapsed() const
				{
					return Clock::now() - tstart;
				}
				double seconds() const
				{
					return elapsed().count() * ((double)Clock::period::num / Clock::period::den);
				}
				void start()
				{
					tstart = Clock::now();
				}
			};

		public:		
			Debugger();

			void Start(int MaxDataPoints);

			void Stop();

			bool IsRunning();

			void CountUp();

			void AddDebugData(vr::HmdVector3d_t Data, int ID);

			void AddDebugData(vr::HmdQuaternion_t Data, int ID);

			void AddDebugData(const double Data[3], int ID);

			bool IsInSync();

			void SetInSync(bool Sync);

			void SetDebugNameQ4(std::string Name, int ID);

			void SetDebugNameV3(std::string Name, int ID);

			void SetZeroPos(vr::HmdVector3d_t vPos, const double vPosRaw[3], vr::HmdQuaternion_t qPos, vr::HmdQuaternion_t qPosRaw);

			void SetLPFValue(double value);

			void WriteFile();

		private:
			int DebugCounter = 0;
			int MaxDebugValues = 10000;
			DebugDataV3_t DebugDataV3[20];
			DebugDataQ4_t DebugDataQ4[5];
			vr::HmdVector3d_t vZeroPos;
			vr::HmdVector3d_t vZeroPosRaw;
			vr::HmdQuaternion_t qZeroPos;
			vr::HmdQuaternion_t qZeroPosRaw;
			double LPFValue;

			timer<boost::chrono::high_resolution_clock> DebugTimer;
			std::vector<double> DebugTiming;

			bool DebuggerRunning = false;
			bool InSync = false;
			bool WroteToFile = false;
		};
	}
}