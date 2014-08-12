/*
 * spidev data transfer tool.
 *
 * (c) 2014 Christophe BLAESS <christophe.blaess@logilin.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

static char * project = "spi-pipe";
static char * version = "0.1";


static void display_usage(const char * name)
{
	fprintf(stderr, "usage: %s options...\n", name);
	fprintf(stderr, "  options:\n");
	fprintf(stderr, "    -d --device=<dev>    use the given spi-dev character device.\n");
	fprintf(stderr, "    -b --blocksize=<int> transfer blocks size in byte\n");
	fprintf(stderr, "    -r --readonly        don't wait for standart input\n");
	fprintf(stderr, "    -h --help            this screen\n");
	fprintf(stderr, "    -v --version         display the version number\n");
}

int main (int argc, char * argv[])
{
	int opt;
	int long_index = 0;

	static struct option options[] = {
		{"device",    required_argument, NULL,  'd' },
		{"blocksize", required_argument, NULL,  'b' },
		{"readonly",  no_argument,       NULL,  'r' },
		{"help",      no_argument,       NULL,  'h' },
		{"version",   no_argument,       NULL,  'v' },
		{0,           0,                 0,  0 }
	};

	char *        device = NULL;
	int           fd;
	uint8_t     * rx_buffer = NULL;
	uint8_t     * tx_buffer = NULL;
	int           blocksize = 1;
	int           readonly = 0;
	int           offset   = 0;
	int           nb       = 0;


	struct spi_ioc_transfer transfer = {
		.tx_buf        = 0,
		.rx_buf        = 0,
		.len           = 0,
		.delay_usecs   = 0,
		.speed_hz      = 0,
		.bits_per_word = 0,
	};


	while ((opt = getopt_long(argc, argv, "d:b:rhv", options, &long_index)) >= 0) {
		switch(opt) {
			case 'r':
				readonly = 1;
				break;
			case 'h':
				display_usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'v':
				fprintf(stderr, "%s - %s\n", project, version);
				exit(EXIT_SUCCESS);
			case 'd':
				device = optarg;
				break;
			case 'b':
				if ((sscanf(optarg, "%d", & blocksize) != 1)
				 || (blocksize < 0) || (blocksize > 16384)) {
					fprintf(stderr, "%s: wrong blocksize\n", argv[0]);
					exit(EXIT_FAILURE);
				}
				break;

			default:
				fprintf(stderr, "%s: wrong option. Use -h for help.\n", argv[0]);
				exit(EXIT_FAILURE);
		}				
	}

	if (((rx_buffer = malloc(blocksize)) == NULL)
	 || ((tx_buffer = malloc(blocksize)) == NULL)) {
		fprintf(stderr, "%s: not enough memory to allocate two %d bytes buffers\n",
		                argv[0], blocksize);
		exit(EXIT_FAILURE);
	}

	memset(rx_buffer, 0, blocksize);
	memset(tx_buffer, 0, blocksize);

	transfer.rx_buf = (unsigned long)rx_buffer;
	transfer.tx_buf = (unsigned long)tx_buffer;
	transfer.len = blocksize;

	if (device == NULL) {
		fprintf(stderr, "%s: no device specified\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		perror(device);
		exit(EXIT_FAILURE);
	}

	while (1) {
		offset = 0;
		if (! readonly) {
			nb = read(STDIN_FILENO, & (tx_buffer[offset]), blocksize - offset);
			if (nb <= 0)
				break;
			offset += nb;
		}

		if (ioctl(fd, SPI_IOC_MESSAGE(1), & transfer) < 0) {
			perror("SPI_IOC_MESSAGE");
			break;
		}
		if (write(STDOUT_FILENO, rx_buffer, blocksize) <= 0)
			break;
	}
	free(rx_buffer);
	free(tx_buffer);
	
	return EXIT_SUCCESS;
}
