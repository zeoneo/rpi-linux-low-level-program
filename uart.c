#include <stdio.h>      // perror
#include <sys/mman.h>   // mmap
#include <sys/types.h>  // fd open
#include <sys/stat.h>   // fd open
#include <fcntl.h>      // fd open
#include <unistd.h>     // fd close

//For RPI 1
// #define BCM2708_PERI_BASE        0x20000000

// For RPI 2, RPI 3
#define BCM2708_PERI_BASE	0x3F000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000)   // GPIO controller
#define UART_BASE			(BCM2708_PERI_BASE + 0x201000)

#define BLOCK_SIZE          (4 * 1024)


// Note that gpio.addr and uart.addr are pointers to unsigned int 
// Hence increment by one result into address 4 Bytes further than current address
// Thats why we need to divide all the offsets by 4.

#define GPPUD				*(gpio.addr + 0x94/4)
#define GPPUDCLK0			*(gpio.addr + 0x98/4)
#define UART_DR				*(uart.addr + 0x00/4)
#define UART_RSRECR			*(uart.addr + 0x04/4)
#define UART_FR				*(uart.addr + 0x18/4)
#define UART_ILPR			*(uart.addr + 0x20/4)
#define UART_IBRD			*(uart.addr + 0x24/4)
#define UART_FBRD			*(uart.addr + 0x28/4)
#define UART_LCRH			*(uart.addr + 0x2C/4)
#define UART_CR				*(uart.addr + 0x30/4)
#define UART_IFLS			*(uart.addr + 0x34/4)
#define UART_IMSC			*(uart.addr + 0x38/4)
#define UART_RIS			*(uart.addr + 0x3C/4)
#define UART_MIS			*(uart.addr + 0x40/4)
#define UART_ICR			*(uart.addr + 0x44/4)
#define UART_DMACR			*(uart.addr + 0x48/4)
#define UART_ITCR			*(uart.addr + 0x80/4)
#define UART_ITIP			*(uart.addr + 0x84/4)
#define UART_ITOP			*(uart.addr + 0x88/4)
#define UART_TDR			*(uart.addr + 0x8c/4)

struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};

struct bcm2835_peripheral gpio = { GPIO_BASE };
struct bcm2835_peripheral uart = { UART_BASE };

// Exposes the physical address defined in the passed structure using mmap on /dev/mem
int map_peripheral(struct bcm2835_peripheral *p)
{
    // Open /dev/mem
    if ((p->mem_fd = open("/dev/mem", O_RDWR | O_SYNC) ) < 0) {
        fprintf(stderr, "Failed to open /dev/mem, try checking permissions.");
        return -1;
    }

    p->map = mmap(
                NULL,
                BLOCK_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                p->mem_fd,      // File descriptor to physical memory virtual file '/dev/mem'
                p->addr_p       // Address in physical map that we want this memory block to expose
                );

    if (p->map == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    p->addr = (volatile unsigned int *)p->map;

    return 0;
}

void unmap_peripheral(struct bcm2835_peripheral *p)
{

    munmap(p->map, BLOCK_SIZE);
    close(p->mem_fd);
}

static inline void delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
                 : "=r"(count)
                 : [count] "0"(count)
                 : "cc");
}

void uart_init() {
	/**-------Procedure to make gpio pins ready to user  -------*/
	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	GPPUD = 0x00000000;
	delay(150);

    // Disable pull up/down for pin 14,15 & delay for 150 cycles.
	GPPUDCLK0 = (1 << 14) | (1 << 15);
	delay(150);
	// Write 0 to GPPUDCLK0 to make it take effect.
	GPPUDCLK0 =  0x00000000;

	/*--------GPIO Initialization over---------------*/

	//Disable UART
	UART_CR = 0x00000000;

	// Clear uart Flag Register
	UART_FR = 0X00000000;

	// Clear pending interrupts.
	UART_ICR =  0x7FF;

	// Set integer & fractional part o baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.
	
	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	UART_IBRD = 1;

	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	UART_FBRD = 40;

	//Clear UART FIFO by writing 0 in FEN bit of LCRH register
	UART_LCRH = (0 << 4);

	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity)
	UART_LCRH = (1 << 4) | (1 << 5) | (1 << 6);

	// Mask all interrupts.
	UART_IMSC = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
                               (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);

    // Enable UART0, receive & transfer part of UART.                  
	UART_CR = (1 << 0) | (1 << 8) | (1 << 9);
}


void uart_putc(unsigned char c)
{
    while (UART_FR & (1 << 5)); // Wait until there is room for new data in Transmission fifo
    UART_DR = (unsigned char) c; // Write data in transmission fifo
}

unsigned char uart_getc()
{
    while (UART_FR & (1 << 4)); // Wait until data arrives in Rx fifo. Bit 4 is set when RX fifo is empty
    return (unsigned char)UART_DR;
}

void uart_puts(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
        uart_putc((unsigned char)str[i]);
}



int main()
{

    if(map_peripheral(&gpio) == -1) {
        fprintf(stderr, "Failed to map the physical GPIO registers into the virtual memory space.\n");
        return -1;
    }

    if(map_peripheral(&uart) == -1) {
        fprintf(stderr, "Failed to map the physical GPIO registers into the virtual memory space.\n");
        return -1;
    }

	unsigned char c;	
	uart_init();

	// Send data to uart
	uart_puts("Hello World \r\n");
	
	// Tell stdout to follow no buffer policy otherwise data won't be printed in terminal until you send LineFeed from other side
	setvbuf(stdout, NULL, _IONBF, 0);

	while(1){
		c = uart_getc();
		printf("%c", c);
		uart_putc(c);
	}

	// Unmap all the physical addresses
	unmap_peripheral(&gpio);
	unmap_peripheral(&uart);
    return 0;
}
