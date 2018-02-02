#pragma once

#define SPI_MSG_POS 1
#define SPI_MSG_COMPASS 2
#define SPI_MSG_GPS 3

typedef float coord_t;

struct SpiMsg
{
	uint8_t data[32];
};

struct PositionRawData
{
	unsigned int id;
	coord_t lon, lat;
	unsigned int time;

	size_t serialize(uint8_t* sm) const;
	size_t deserialize(const uint8_t* sm);
};

struct CompassRawData
{
	unsigned int dir;

	size_t serialize(uint8_t* sm) const;
	size_t deserialize(const uint8_t* sm);
};
