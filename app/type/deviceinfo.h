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

typedef enum {
	DETECTOR_COLI = 0x01,
	DETECTOR_VIDEO = 0x02,
	DETECTOR_RFID = 0x07,
	DETECTOR_OTHER = 0x09
} szzt_DetectorType;

typedef enum {
	EAST = 0x01,
	WEST = 0x02,
	SOUTH = 0x03,
	NORTH = 0x04
} szzt_Location;

typedef enum {
	LEFT = 0x01,
	STRAIGHT = 0x02,
	RIGHT = 0x03
} szzt_Direction;

typedef enum {
	COMTYPE_TCPIP = 0x01,
	COMTYPE_ZIGBEE,
	COMTYPE_RS232,
	COMTYPE_RS485
} szzt_DetectorComType;

typedef enum {
	FRONT_DATA = 0x01,
	TACTICS_DATA,
	POLICY_DATA
} szzt_DetectorAttr;


struct SzDetector
{	
	uint8_t id;			
	uint8_t type;
	uint8_t comType;
	std::string comIp;
	uint16_t comPort;
	uint16_t comAddr;

	struct LogicLane
	{
		uint8_t logicLaneId;
		uint8_t dataAttr;
		uint8_t direction = 0;
	};
	std::list<LogicLane> laneChains;
	uint16_t collectCycle = 0;
	uint8_t  noResponse;
	uint8_t	 maxContiunous;
};

typedef std::vector<SzDetector> SzDetectorTable;


struct LaneInfo
{
	uint8_t id = 0;
	uint8_t channel = 0;
	uint16_t flow = 0;
	std::string status;
};
typedef std::vector<LaneInfo> SzLaneInfoTable;


struct TscDeviceInfo
{
	std::string company;
	std::string devVersion;
	std::string factoryDate;
								
	uint8_t	type;
	bool    sec;
};