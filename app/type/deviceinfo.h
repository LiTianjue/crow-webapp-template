#include <stdint.h>
#include <string>
#include <vector>
#include <list>

struct DeviceName
{
	std::string deviceName = "TSC400";
	std::string	manufacturer = "Hikvision";
	std::string software = "V4.0.1";
};

struct LaneInfo
{
	uint8_t id = 0;
	uint8_t channel = 0;
	uint16_t flow = 0;
	std::string status;
};
typedef std::vector<LaneInfo> SzLaneInfoTable;
