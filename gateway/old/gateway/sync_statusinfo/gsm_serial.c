#ifndef _GNU_SOURCE
// To get ptsname grandpt and unlockpt definitions from stdlib.h
#define _GNU_SOURCE
#endif
#include "gsm_serial.h"


/* The following arrays must have equal length and the values must 
 * correspond.
 */
static int baudrates[] = { 
    0, 9600, 19200, 38400, 57600, 115200, 230400, 460800 };

static speed_t baud_bits[] = {
    0, B9600, B19200, B38400, B57600, B115200, B230400, B460800 };



/**
 * Determine baud rate index for CMUX command
 */
int indexOfBaud(int baudrate) {
    int i;

    for (i = 0; i < sizeof(baudrates) / sizeof(baudrates[0]); ++i) {
        if (baudrates[i] == baudrate)
            return i;
    }
    return 0;
}

int open_serialport(char *dev)
{
    return open_serialport_withbaud(dev, 115200);
}

/* Opens serial port, set's it to 57600bps 8N1 RTS/CTS mode.
*
* PARAMS:
* dev - device name
* RETURNS :
* file descriptor or -1 on error
*/
int open_serialport_withbaud(char *dev, speed_t baud)
{
	int fd;

	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd != -1)
	{
		int index = indexOfBaud(baud);
		if (index > 0) {
			// Switch the baud rate to zero and back up to wake up 
			// the modem
			setAdvancedOptions(fd, baud_bits[index]);
		} else {
			struct termios options;
			// The old way. Let's not change baud settings
			fcntl(fd, F_SETFL, 0);
			
			// get the parameters
			tcgetattr(fd, &options);
			
			// Set the baud rates to 57600...
			// cfsetispeed(&options, B57600);
			// cfsetospeed(&options, B57600);
			
			// Enable the receiver and set local mode...
			options.c_cflag |= (CLOCAL | CREAD);
			
			// No parity (8N1):
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			options.c_cflag &= ~CSIZE;
			options.c_cflag |= CS8;
			
			// enable hardware flow control (CNEW_RTCCTS)
			// options.c_cflag |= CRTSCTS;
			
			// set raw input
			options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
			options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
			
			// set raw output
			options.c_oflag &= ~OPOST;
			options.c_oflag &= ~OLCUC;
			options.c_oflag &= ~ONLRET;
			options.c_oflag &= ~ONOCR;
			options.c_oflag &= ~OCRNL;
			
			// Set the new options for the port...
			tcsetattr(fd, TCSANOW, &options);
		}
	}
	return fd;
}

void close_serialport(int fd)
{
	close(fd);
}


/** 
 * Set serial port options. Then switch baudrate to zero for a while
 * and then back up. This is needed to get some modems 
 * (such as Siemens MC35i) to wake up.
 */
void setAdvancedOptions(int fd, speed_t baud) {
    struct termios options;
    struct termios options_cpy;

    fcntl(fd, F_SETFL, 0);
    
    // get the parameters
    tcgetattr(fd, &options);
    
    // Do like minicom: set 0 in speed options
    cfsetispeed(&options, 0);
    cfsetospeed(&options, 0);
    
    options.c_iflag = IGNBRK;
    
    // Enable the receiver and set local mode and 8N1
    options.c_cflag = (CLOCAL | CREAD | CS8 | HUPCL);
    // enable hardware flow control (CNEW_RTCCTS)
    // options.c_cflag |= CRTSCTS;
    // Set speed
    options.c_cflag |= baud;
    
    /*
      options.c_cflag &= ~PARENB;
      options.c_cflag &= ~CSTOPB;
      options.c_cflag &= ~CSIZE; // Could this be wrong!?!?!?
    */
    
    // set raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    
    // set raw output
    options.c_oflag &= ~OPOST;
    options.c_oflag &= ~OLCUC;
    options.c_oflag &= ~ONLRET;
    options.c_oflag &= ~ONOCR;
    options.c_oflag &= ~OCRNL;
    
    // Set the new options for the port...
    options_cpy = options;
    tcsetattr(fd, TCSANOW, &options);
    options = options_cpy;
    
    // Do like minicom: set speed to 0 and back
    options.c_cflag &= ~baud;
    tcsetattr(fd, TCSANOW, &options);
    options = options_cpy;

    options.c_cflag |= baud;
    tcsetattr(fd, TCSANOW, &options);
}
