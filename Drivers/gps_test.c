#include <stdint.h>
#include <stddef.h>
#include "log.h"
#include "uart.h"
#include "pinmux.h"
#include "traps.h"
#include "platform.h"
#include "plic_driver.h"
#include "GPS.h"

#define UART_INST_0 uart_instance[0]
#define UART_INST_1 uart_instance[1]
#define UART_INST_2 uart_instance[2]

#define USE_INTERRUPT

extern volatile unsigned int* pinmux_config_reg;


GPS_Data_TypeDef GPSData;                   // Parsed GPS information
bool GPS_new_data;                          // TRUE if received new GPS packet
bool GPS_parsed;                            // TRUE if GPS packets was parsed
uint16_t GPS_buf_cntr;                      // Number of actual bytes in GPS buffer
NMEASentence_TypeDef GPS_msg;               // NMEA sentence position
uint8_t GPS_sentences_parsed;               // Parsed NMEA sentences counter
uint8_t GPS_sentences_unknown;              // Found unknown NMEA sentences counter
uint8_t GPS_buf[GPS_BUFFER_SIZE];           // Buffer with data from GPS
uint8_t GPS_sats[12];                       // IDs of satellites used in position fix
// Information about satellites in view (can be increased if receiver able handle more)
GPS_Satellite_TypeDef GPS_sats_view[MAX_SATELLITES_VIEW];
extern Cur_Data_TypeDef CurData;                   // Current data (Speed, Cadence, etc.)


void init_variables_uart0()
{
    uart0_complete = 0;
	uart0_tx_isr_count = 0;
	uart0_rcv_isr_count = 0;
}

void init_variables_uart1()
{
    uart1_complete = 0;
	uart1_tx_isr_count = 0;
	uart1_rcv_isr_count = 0;
	configure_interrupt(PLIC_INTERRUPT_26);
	isr_table[PLIC_INTERRUPT_26] = uart1_isr;
}

void init_variables_uart2()
{
    uart2_complete = 0;
	uart2_tx_isr_count = 0;
	uart2_rcv_isr_count = 0;
	configure_interrupt(PLIC_INTERRUPT_27);
	isr_table[PLIC_INTERRUPT_27] = uart2_isr;
}

int uart_r_data(uart_struct * uart_instance, uint8_t *str,int cnt)
{
   int i;
   for(i=0;i<cnt;i++)
   {
      str[i]=u2_rcv_char[i];
   }
   uart2_rcv_isr_count = 0;
   memset(u2_rcv_char,'\0', UARTX_BUFFER_SIZE);
   return 1;
}


/*****************************************************************************
** Function name:		PutTerminalCharacter
**
** Description:			This function writes a string, one character at a time, to
**                  a USART. The string MUST be terminated by a terminating
**                  NUL ("\0") character. Both the USART instance and the string
**                  are passed into the function by reference using pointers.
**
** parameters:			ptr_LPC_USART: A pointer to any USART on the target.
**                  ptr_string: A pointer to an array of char terminated by NUL.
**
** Returned value:	void
**
*****************************************************************************/


// Convert characters to number with fixed length
// input:
//   buf - point to character buffer
//   len - length of number to convert
// return: converted number
// note: only positive values
uint32_t atos_len(uint8_t *buf, uint8_t len) {
	uint32_t value = 0;
	uint8_t digit;
	uint8_t i;
	char c;

	for (i = 0; i < len; i++) {
		c = buf[i];
		if (c >= '0' && c <= '9') digit = (uint8_t)(c - '0'); else return 0;
		value = (value * 10) + digit;
    }

    return value;
}

// Convert characters to number
// from specified position until first non digital character
// input:
//   buf - point to character buffer
//   pos - point to position inside buffer
// return: converted number
// note: pos value will be changed to the next character after converted number
int32_t atos_char(uint8_t *buf, uint16_t *pos) {
	uint32_t value = 0;
	uint8_t digit;
	uint8_t i = 0;
	int8_t neg = 1;
	char c;

	if (buf[i] == '-') {
		neg = -1;
		i++;
	}
	for (; i < 11; i++) {
		c = buf[i];
		if (c >= '0' && c <= '9') digit = (uint8_t)(c - '0'); else {
			*pos += i + 1;
			return value * neg;
		}
		value = (value * 10) + digit;
    }
	*pos += i + 1;

    return value * neg;
}



// Calculates ten raised to the given power
// input:
//   exp - exponent value
// return: ten in raised to the exponent
uint32_t pwr10(uint32_t exp) {
	uint32_t result = 1;

	while (exp--) result *= 10;

	return result;
}

// Calculate CRC of NMEA command
// input:
//   str - pointer to the string with command
// return:
//   checksum of command
// note: command must begin with '$' char and end with '*' or zero symbol
uint8_t GPS_CRC(char *str) {
	uint8_t result = 0;

	if (*str++ == '$') while (*str != '*' && *str != '\0') result ^= *str++;

	return result;
}

// Send NMEA command
// input:
//   cmd - pointer to string with command
// note: command must begin with '$' char and end with '*'


// Find end of the GPS sentence
// input:
//   buf - pointer to the data buffer
//   buf_size - size of the data buffer
// return: buffer offset
uint16_t GPS_FindSentenceTail(uint8_t *buf, uint16_t buf_size) {
	uint16_t pos = 0;
	uint16_t crlf;

	do {
		crlf = (buf[pos] << 8) | buf[pos + 1];
		pos++;
	} while (crlf != 0x0d0a && (pos + 1) < buf_size);

	return pos;
}

// Find next field in GPS sentence
// input:
//   buf - pointer to the data buffer
// return: buffer offset of next field
uint16_t GPS_NextField(uint8_t *buf) {
	uint16_t pos = 1;

	while (*buf != ',' && *buf++ != '*') pos++;

	return pos;
}

// Find NMEA sentence in buffer
// input:
//   msg - pointer to NMEASentence variable
//   buf - pointer to buffer with NMEA packet
//   start - position in buffer to start search
//   buf_size - size of the data buffer
// note: function modifies msg variable
void GPS_FindSentence(NMEASentence_TypeDef *Sentence, uint8_t *buf, uint16_t start, uint16_t buf_size) {
	uint16_t pos = start;
	uint32_t hdr;

	Sentence->start = start;
	Sentence->end   = buf_size;
	Sentence->type  = NMEA_BAD;

	if (start + 10 > buf_size) return;

	do 
	{
		hdr = (buf[pos] << 16) | (buf[pos + 1] << 8) | buf[pos + 2];
		if (hdr == 0x00244750 | hdr == 0x0024474E) {
			// $GPxxx sentence - GPS
			Sentence->start = pos + 7;
			hdr = (buf[pos + 3] << 16) |(buf[pos + 4] << 8)  | buf[pos + 5];

			pos += GPS_FindSentenceTail(&buf[pos],buf_size);
			if (pos >= buf_size) return;
			Sentence->end = pos + 1;
			if (hdr == 0x00474c4c) { Sentence->type = NMEA_GLL; return; }
			if (hdr == 0x00524d43) { Sentence->type = NMEA_RMC; return; }
			if (hdr == 0x00565447) { Sentence->type = NMEA_VTG; return; }
			if (hdr == 0x00474741) { Sentence->type = NMEA_GGA; return; }
			if (hdr == 0x00475341) { Sentence->type = NMEA_GSA; return; }
			if (hdr == 0x00475356) { Sentence->type = NMEA_GSV; return; }
			if (hdr == 0x005a4441) { Sentence->type = NMEA_ZDA; return; }
			return;
		} 
		pos++;
	} while (pos < buf_size - 3);

	return;
}

// Parse float value from a GPS sentence
// input:
//   buf - pointer to the data buffer
//   value - pointer to parsed value (float represented as integer, e.g. 1234.567 -> 1234567)
// return: number of parsed bytes
uint16_t GPS_ParseFloat(uint8_t *buf, uint32_t *value) {
	uint16_t pos = 0;
	uint32_t ip;
	uint16_t len;

	if (*buf == ',' || *buf == '*') {
		*value = 0;
		return 1;
	}
	ip = atos_char(&buf[pos],&pos); // integer part
	if (buf[pos - 1] == '.') {
		// fractional part
		len = pos;
		*value  = atos_char(&buf[pos],&pos);
		*value += ip * pwr10(pos - len - 1);
	} else {
		// this value is not float
		*value = ip;
	}

	return pos;
}

// Parse latitude or longitude coordinate
// input:
//   buf - pointer to the data buffer
//   len - length of the degrees value (2 for latitude, 3 for longitude)
//   value - pointer to the coordinate variable
//   char_value - pointer to the coordinate character variable
// return: number of parsed bytes
uint16_t GPS_ParseCoordinate(uint8_t *buf, uint8_t len, uint32_t *value, uint8_t *char_value) {
	uint16_t pos = 0;
	uint32_t coord_minutes;
	int16_t f_len; // fractional part length

	// Coordinate
	if (buf[pos] != ',') {
		// '1000000' determines length of the fractional part in result
		// e.g. 1000000 means 6 fractional digits
		*value = atos_len(&buf[pos],len) * 1000000; // degrees
		pos += len;
		f_len = pos;
		pos += GPS_ParseFloat(&buf[pos],&coord_minutes); // minutes
		f_len = pos - f_len - 4; // fractional part length
		if (f_len > 0) {
			// Float calculations, slow
			*value += (uint32_t)((coord_minutes / (pwr10(f_len) * 60.0)) * 1000000);
		} else {
			// Are you serious? Floating part is mandatory!
			*value += coord_minutes * 100;
		}
	} else pos++;

	// Coordinate character
	if (buf[pos] != ',') {
		*char_value = buf[pos];
		pos += 2;
	} else {
		*char_value = 'X';
		pos++;
	}

	// FIXME: coordinate must be negative in case when char_value is 'W' or 'S'

	return pos;
}

// Parse one satellite from $GPGSV sentence
// input:
//   buf - pointer to the data buffer
//   set_num - satellite number in GPS_sats_view[]
// return: number of parsed bytes
uint16_t GPS_ParseSatelliteInView(uint8_t *buf, uint8_t sat_num) {
	uint16_t pos = 0;

	// Satellite PRN number
	if (buf[pos] != ',') {
		GPS_sats_view[sat_num].PRN = atos_len(&buf[pos],2);
		pos += 3;
	} else {
		GPS_sats_view[sat_num].PRN = 0;
		pos++;
	}

	// Satellite elevation
	if (buf[pos] != ',') {
		GPS_sats_view[sat_num].elevation = atos_len(&buf[pos],2);
		pos += 3;
	} else {
		GPS_sats_view[sat_num].elevation = 0;
		pos++;
	}

	// Satellite azimuth
	if (buf[pos] != ',') {
		GPS_sats_view[sat_num].azimuth = atos_len(&buf[pos],3);
		pos += 4;
	} else {
		GPS_sats_view[sat_num].azimuth = 0;
		pos++;
	}

	// Satellite SNR
	if (buf[pos] != ',' && buf[pos] != '*') {
		GPS_sats_view[sat_num].SNR = atos_len(&buf[pos],2);
		pos += 3;
	} else {
		GPS_sats_view[sat_num].SNR = 255; // Satellite not tracked
		pos++;
	}

	// This must be set after all GPS sentences parsed
	GPS_sats_view[sat_num].used = FALSE;

	return pos;
}

// Parse time from NMEA sentence
// input:
//   buf - pointer to the data buffer
//   time - pointer to variable where time will be stored
// return: number of parsed bytes
uint16_t GPS_ParseTime(uint8_t *buf, uint32_t *time) {
	uint16_t pos = 0;

	// Parse time
	*time  = atos_len(&buf[pos],2) * 3600;
	pos += 2;
	*time += atos_len(&buf[pos],2) * 60;
	pos += 2;
	*time += atos_len(&buf[pos],2);
	pos += 3;
	pos += GPS_NextField(&buf[pos]); // Ignore milliseconds

	return pos;
}

// Parse NMEA sentence
// input:
//   buf - pointer to the data buffer
//   Sentence - pointer to the structure with NMEA sentence parameters
void GPS_ParseSentence(uint8_t *buf, NMEASentence_TypeDef *Sentence) {
	uint16_t pos = Sentence->start;
	uint8_t i;
	uint8_t GSV_msg;   // GSV sentence number
	uint8_t sat_num;   // Satellites quantity in sentence
	uint8_t GSV_sats;  // Total number of satellites in view
	uint32_t ui_32;
	uint16_t ui_16;
	switch (Sentence->type) 
	{
	case NMEA_RMC:
		// $GPRMC - Recommended minimum specific GPS/Transit data

		// Time of fix
		if (buf[pos] != ',') pos += GPS_ParseTime(&buf[pos],&GPSData.fix_time); else pos++;

		// Valid data marker
		GPSData.valid = FALSE;
		if (buf[pos] != ',') {
			if (buf[pos] == 'A') GPSData.valid = TRUE;
			pos += 2;
		} else pos++;

		// Latitude
		pos += GPS_ParseCoordinate(&buf[pos],2,&GPSData.latitude,&GPSData.latitude_char);

		// Longitude
		pos += GPS_ParseCoordinate(&buf[pos],3,&GPSData.longitude,&GPSData.longitude_char);

		// Horizontal speed (in knots)
		pos += GPS_ParseFloat(&buf[pos],&GPSData.speed_k);

		// Convert speed in knots to speed in km/h
		if (GPSData.speed == 0 && GPSData.speed_k != 0) GPSData.speed = (GPSData.speed_k * 1852) / 1000;

		// Course
		pos += GPS_ParseFloat(&buf[pos],&GPSData.course);

		// Date of fix
		if (buf[pos] != ',') {
			GPSData.fix_date  = atos_len(&buf[pos],2) * 1000000;
			GPSData.fix_date += atos_len(&buf[pos + 2],2) * 100000;
			GPSData.fix_date += atos_len(&buf[pos + 4],2) + 2000;
			pos += 6;
		} pos++;

		// Magnetic variation
		// ignore this (mostly not supported by GPS receivers)
		pos += GPS_NextField(&buf[pos]);

		// Magnetic variation direction
		// ignore this (mostly not supported by GPS receivers)
		pos += GPS_NextField(&buf[pos]);

		// Mode indicator (NMEA 0183 v3.0 or never)
		if (buf[pos] != ',' && buf[pos] != '*') GPSData.mode = buf[pos];

		break; // NMEA_RMC
	case NMEA_GLL:
		// $GPGLL - Geographic position, latitude / longitude

		// Latitude
		pos += GPS_ParseCoordinate(&buf[pos],2,&GPSData.latitude,&GPSData.latitude_char);

		// Longitude
		pos += GPS_ParseCoordinate(&buf[pos],3,&GPSData.longitude,&GPSData.longitude_char);

		// Time of fix
		if (buf[pos] != ',') pos += GPS_ParseTime(&buf[pos],&GPSData.fix_time); else pos++;

		// Valid data marker
		GPSData.valid = FALSE;
		if (buf[pos] != ',') {
			if (buf[pos] == 'A') GPSData.valid = TRUE;
			pos += 2;
		} else pos++;

		// Mode indicator
		if (buf[pos] != ',') GPSData.mode = buf[pos]; else GPSData.mode = 'N';

		break; // NMEA_GLL
	case NMEA_ZDA:
		// $GPZDA - Date & Time

		// Time
		if (buf[pos] != ',') pos += GPS_ParseTime(&buf[pos],&GPSData.time); else pos++;

		// Date: day
		if (buf[pos] != ',') {
			GPSData.date = atos_len(&buf[pos],2) * 1000000;
			pos += 3;
		} else {
			GPSData.date = 1000000;
			pos++;
		}

		// Date: month
		if (buf[pos] != ',') {
			GPSData.date += atos_len(&buf[pos],2) * 10000;
			pos += 3;
		} else {
			GPSData.date += 10000;
			pos++;
		}

		// Date: year
		if (buf[pos] != ',') {
			GPSData.date += atos_len(&buf[pos],4);
		} else GPSData.date += 2013;

		// Local time zone offset
		// sad but true: this feature mostly not supported by GPS receivers

		// Check for year, if it less than 2014, the date from the GPS receiver is not valid
		if (GPSData.date % 10000 > 2013) GPSData.datetime_valid = TRUE;

		break; // NMEA_ZDA
	case NMEA_VTG:
		// $GPVTG - Course over ground and ground speed

		// Course (heading relative to true north)
		pos += GPS_ParseFloat(&buf[pos],&GPSData.course);

		// Field with 'T' letter - "track made good is relative to true north"
		// ignore it
		pos += GPS_NextField(&buf[pos]);

		// Field with course relative to magnetic north
		// mostly not supported by GPS receivers, ignore it
		pos += GPS_NextField(&buf[pos]);

		// Field with 'M' letter - "track made good is relative to magnetic north"
		// mostly not supported by GPS receivers, ignore it
		pos += GPS_NextField(&buf[pos]);

		// Speed over ground in knots
		pos += GPS_ParseFloat(&buf[pos],&GPSData.speed_k);

		// Convert speed in knots to speed in km/h
		if (GPSData.speed == 0 && GPSData.speed_k != 0) GPSData.speed = (GPSData.speed_k * 1852) / 1000;

		// Field with 'N' - speed over ground measured in knots, ignore it
		pos += GPS_NextField(&buf[pos]);

		// Speed over ground in km/h
		pos += GPS_ParseFloat(&buf[pos],&GPSData.speed);

		// Field with 'K' - speed over ground measured in km/h, ignore it
		pos += GPS_NextField(&buf[pos]);

		// Mode indicator (NMEA 0183 v3.0 or later)
		if (buf[pos] != ',' && buf[pos] != '*') GPSData.mode = buf[pos]; else GPSData.mode = 'N';

		break; // NMEA_VTG
	case NMEA_GGA:
		// $GPGGA - GPS fix data

		// Time
		if (buf[pos] != ',') pos += GPS_ParseTime(&buf[pos],&GPSData.time); else pos++;

		// Latitude
		pos += GPS_ParseCoordinate(&buf[pos],2,&GPSData.latitude,&GPSData.latitude_char);

		// Longitude
		pos += GPS_ParseCoordinate(&buf[pos],3,&GPSData.longitude,&GPSData.longitude_char);

		// Position fix indicator
		if (buf[pos] != ',') {
			GPSData.fix_quality = buf[pos] - '0';
			pos += 2;
		} else pos++;

		// Satellites used
		if (buf[pos] != ',') GPSData.sats_used = atos_char(&buf[pos],&pos); else pos++;

		// HDOP - horizontal dilution of precision
		pos += GPS_ParseFloat(&buf[pos],&GPSData.HDOP);

		// MSL Altitude (mean-sea-level)
		if (buf[pos] != ',') {
			// This value can be negative
			// Only integer part, fractional is useless
			GPSData.altitude = atos_char(&buf[pos],&pos);
			pos += GPS_NextField(&buf[pos]);
/*
			GPSData.altitude  = atos_char(&buf[pos],&pos) * 1000;
			if (GPSData.altitude >= 0)
				GPSData.altitude += atos_len(&buf[pos],3);
			else
				GPSData.altitude -= atos_len(&buf[pos],3);
			pos += 4;
*/
		} else pos++;

		// Altitude measurement units
		// ignore this field and assume what units is meters
		buf += GPS_NextField(&buf[pos]);

		// Geoid-to-ellipsoid separation (Ellipsoid altitude = MSL altitude + Geoid separation)
		if (buf[pos] != ',') {
			// This value can be negative
			GPSData.geoid_separation = atos_char(&buf[pos],&pos) * 1000;
			if (GPSData.geoid_separation >= 0)
				GPSData.geoid_separation += atos_char(&buf[pos],&pos);
			else
				GPSData.geoid_separation -= atos_char(&buf[pos],&pos);
		} else pos++;

		// Time since last DGPS update
		if (buf[pos] != ',') {
			GPSData.dgps_age = atos_char(&buf[pos],&pos);
		} else pos++;

		// DGPS station ID
		if (buf[pos] != ',' && buf[pos] != '*') {
			GPSData.dgps_id = atos_char(&buf[pos],&pos);
		}

		break; // NMEA_GGA
	case NMEA_GSA:
		// $GPGSA - GPS DOP and active satellites

		// Satellite acquisition mode (M = manually forced 2D or 3D, A = automatic switch between 2D and 3D)
		// ignore this field
		pos += GPS_NextField(&buf[pos]);

		// Position mode (1=fix not available, 2=2D fix, 3=3D fix)
		if (buf[pos] != ',') {
			GPSData.fix = buf[pos] - '0';
			pos += 2;
		} else {
			GPSData.fix = 1;
			pos++;
		}

		// IDs of satellites used in position fix (12 fields)
		for (i = 0; i < 12; i++) {
			if (buf[pos] != ',') {
				GPS_sats[i] = atos_len(&buf[pos],2);
				pos += 3;
			} else {
				GPS_sats[i] = 0;
				pos++;
			}
		}

		// PDOP - position dilution
		// In theory this thing must be equal to SQRT(pow(HDOP,2) + pow(VDOP,2))
		pos += GPS_ParseFloat(&buf[pos],&GPSData.PDOP);
		GPSData.accuracy = GPSData.PDOP * GPS_DOP_FACTOR;

		// HDOP - horizontal position dilution
		pos += GPS_ParseFloat(&buf[pos],&GPSData.HDOP);

		// VDOP - vertical position dilution
		pos += GPS_ParseFloat(&buf[pos],&GPSData.VDOP);

		break; // NMEA_GSA
	case NMEA_GSV:
		// $GPGSV - GPS Satellites in view

		// Field with "total number of GSV sentences in this cycle"
		// ignore it
		pos += GPS_NextField(&buf[pos]);

		// GSV sentence number
		if (buf[pos] != ',') {
			GSV_msg = atos_len(&buf[pos],1);
			pos += 2;
		} else {
			pos++;
			GSV_msg = 0;
		}

		// Total number of satellites in view
		if (buf[pos] != ',') {
			GSV_sats = atos_len(&buf[pos],2);
			pos += 3;
		} else {
			GSV_sats = 0;
			pos++;
		}

		GPSData.sats_view = GSV_sats;

		// Parse no more than 12 satellites in view
		sat_num = (GSV_msg - 1) * 4;
		if (GSV_sats != 0 && sat_num < MAX_SATELLITES_VIEW) {
			// 4 satellites per sentence
			pos += GPS_ParseSatelliteInView(&buf[pos],sat_num++);
			pos += GPS_ParseSatelliteInView(&buf[pos],sat_num++);
			pos += GPS_ParseSatelliteInView(&buf[pos],sat_num++);
			pos += GPS_ParseSatelliteInView(&buf[pos],sat_num++);
		}

		break; // NMEA_GSV

	default:
		// Unknown NMEA sentence
		break;
	}
}

// Initialize GPSData variable
void GPS_InitData(void) 
{
	uint32_t i;

	memset(&GPSData,0,sizeof(GPSData));
	for (i = 0; i < 12; i++) GPS_sats[i] = 0;
	for (i = 0; i < MAX_SATELLITES_VIEW; i++) 
	{
		memset(&GPS_sats_view[i],0,sizeof(GPS_Satellite_TypeDef));
		GPS_sats_view[i].SNR = 255;
	}
	GPSData.longitude_char = 'X';
	GPSData.latitude_char  = 'X';
	GPSData.mode = 'N';

	GPS_sentences_parsed = 0;
	GPS_sentences_unknown = 0;

	memset(&GPS_msg,0,sizeof(GPS_msg));
}

// Check which satellites in view is used in location fix
void GPS_CheckUsedSats(void) {
	uint32_t i,j;

	for (i = 0; i < GPSData.sats_view; i++) {
		GPS_sats_view[i].used = FALSE;
		for (j = 0; GPSData.sats_used; j++) {
			if (GPS_sats[j] == GPS_sats_view[i].PRN) {
				GPS_sats_view[i].used = TRUE;
				break;
			}
		}
	}
}



// Parse GPS data
void ParseGPS(uint8_t *buf) {
//	BEEPER_Enable(222,1);
	GPS_InitData(); // Clear previously parsed GPS data

    GPS_msg.end = 0;
    GPS_buf_cntr = 1023;

	while (GPS_msg.end < GPS_buf_cntr) 
	{
		GPS_FindSentence(&GPS_msg, buf, GPS_msg.end,GPS_buf_cntr);
		if (GPS_msg.type != NMEA_BAD) 
		{
			GPS_sentences_parsed++;
			GPS_ParseSentence(buf,&GPS_msg);
		} 
		else 
		    GPS_sentences_unknown++;
	}
	GPS_buf_cntr = 0;
	if (GPS_sentences_parsed) 
	{
		GPS_CheckUsedSats();
		if (GPSData.fix == 3) 
		{
			// GPS altitude makes sense only in case of 3D fix
			CurData.GPSAlt = GPSData.altitude;
			if (CurData.GPSAlt > CurData.MaxGPSAlt) CurData.MaxGPSAlt = CurData.GPSAlt;
			if (CurData.GPSAlt < CurData.MinGPSAlt) CurData.MinGPSAlt = CurData.GPSAlt;
		}
		if (GPSData.fix == 2 || GPSData.fix == 3) 
		{
			// GPS speed makes sense only in case of 2D or 3D position fix
			CurData.GPSSpeed = GPSData.speed;
			if (CurData.GPSSpeed > CurData.MaxGPSSpeed) CurData.MaxGPSSpeed = CurData.GPSSpeed;
		}
		GPS_new_data = FALSE;
		GPS_parsed = TRUE;
	}
}
}

int main()
 {	
   *pinmux_config_reg = 0x55;
   uint8_t *buf;
   set_baud_rate(UART_INST_1, 9600); 
   set_baud_rate(UART_INST_2, 9600); 
   plic_init();
   init_variables_uart1();
   init_variables_uart2();
   flush_uart(UART_INST_1);
   flush_uart(UART_INST_2);
   register uint32_t retval;

	// Enable Global (PLIC) interrupts.
	asm volatile("li      t0, 8\t\n"
		     "csrrs   zero, mstatus, t0\t\n"
		    );

	asm volatile("li      t0, 0x800\t\n"
		     "csrrs   zero, mie, t0\t\n"
		    );

	asm volatile(
		     "csrr %[retval], mstatus\n"
		     :
		     [retval]
		     "=r"
		     (retval)
		    );

	asm volatile(
		     "csrr %[retval], mie\n"
		     :
		     [retval]
		     "=r"
		     (retval)
		    );

	asm volatile(
		     "csrr %[retval], mip\n"
		     :

		     [retval]
		     "=r"
		     (retval)
		    );

   flush_uart(UART_INST_1);
   flush_uart(UART_INST_2);
   enable_uart_interrupts(UART_INST_1,  ENABLE_RX_NOT_EMPTY);    // | ENABLE_RX_FULL
   enable_uart_interrupts(UART_INST_2,  ENABLE_RX_NOT_EMPTY);

   uint32_t longitude,latitude;
   
   while(1)
   {
       while(uart1_rcv_isr_count < 1024);
       //uart_r_data(UART_INST_1,buf);
       ParseGPS(u1_rcv_char);
	   if(GPSData.valid==TRUE)
	   {
		   latitude=GPSData.latitude;
		   longitude=GPSData.longitude;
		   log_info("Lati:%d Long:%d",latitude,longitude);
	   }
       latitude=GPSData.latitude;
       longitude=GPSData.longitude;
       log_info("Lati:%d Long:%d",latitude,longitude);
   }
   return 0; 
 }
