#include "stdafx.h"
#include "Protocol.h"

template<typename T>
size_t serial_put(uint8_t* pos, T v)
{
	*(T*)pos = v;
	return sizeof(T);
}

template<typename T>
size_t serial_get(const uint8_t* pos, T* v)
{
	*v = *(const T*)pos;
	return sizeof(T);
}

size_t PositionRawData::serialize(uint8_t* dst) const
{
	size_t n = 0;
	n += serial_put<uint32_t>(dst + n, id);
	n += serial_put<coord_t>(dst + n, lon);
	n += serial_put<coord_t>(dst + n, lat);
	n += serial_put<uint32_t>(dst + n, time);
	// TODO: checksum
	return n;
}

size_t PositionRawData::deserialize(const uint8_t* src)
{
	size_t n = 0;
	n += serial_get<uint32_t>(src + n, &id);
	n += serial_get<coord_t>(src + n, &lon);
	n += serial_get<coord_t>(src + n, &lat);
	n += serial_get<uint32_t>(src + n, &time);
	// TODO: checksum
	return n;
}

size_t CompassRawData::serialize(uint8_t* dst) const
{
	size_t n = 0;
	n += serial_put<uint32_t>(dst + n, dir);
	// TODO: checksum
	return n;
}

size_t CompassRawData::deserialize(const uint8_t* src)
{
	size_t n = 0;
	n += serial_get<uint32_t>(src + n, &dir);
	// TODO: checksum
	return n;
}
