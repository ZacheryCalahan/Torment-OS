/*
*   Related documents: 
*   1: https://www.redhat.com/en/blog/virtio-devices-and-drivers-overview-headjack-and-phone
*   2: https://www.redhat.com/en/blog/virtqueues-and-virtio-ring-how-data-travels
*   3: https://www.redhat.com/en/blog/packed-virtqueue-how-reduce-overhead-virtio
*
*   1: https://blogs.oracle.com/linux/post/introduction-to-virtio (includes an example for SCSI, or hdd access.)
*
*   Documentations: 
*   1: http://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.pdf
*/

#ifndef VIRTIO_H
#define VIRTIO_H
#include <stdint.h>
#include <stddef.h>

// Device Status Registers (MMIO)

#define VIRTR_MAGIC_O               0x000   // (R) 0x74726976 magic number for all virtio devices.
#define VIRTR_VERSION_O             0x004   // (R) Device version number. Must return 0x2 to be valid.
#define VIRTR_DEV_ID_O              0x008   // (R) Virtio Subsystem device ID.
#define VIRTR_VEND_ID_O             0x00C   // (R) Virtio Subsystem vendor ID.
#define VIRTR_DEV_FEATURES_O        0x010   // (R) Flags of features a device supports.
#define VIRTR_DEV_FEATURES_SEL_O    0x014   // (W) Writing to this register selects a set of 32 device feature bits accessible by reading from VITR_DEV_FEATURES.
#define VIRTR_DRV_FEATURES_O        0x020   // (W) Flags representing device features understood and activated by the driver.
#define VIRTR_DRV_FEATURES_SEL_0    0x024   // (W) Flags the driver states can support (Write here to submit features supported).
#define VIRTR_QUEUE_SEL_O           0x030   // (W) Selects the virtual queue.
#define VIRTR_QUEUE_NUM_MAX_O       0x034   // (R) Returns the max number of elements of the queue the device is ready to process, or 0x0 if the queue is not available.
#define VIRTR_QUEUE_NUM_O           0x038   // (W) Tells device of the number of elements (max elements) in the queue the driver will use.
#define VIRTR_QUEUE_READY_O         0x044   // (RW) Writing 0x1 to this register notifies the device it can execute requests from the virtual queue. Reading this register returns the last value written to it.
#define VIRTR_QUEUE_NOTIFY_O        0x050   // (W) Writing the this register notifies the device there are new buffers to process in a queue.
#define VIRTR_INT_STATUS_O          0x060   // (R) Returns a bit mask of events that caused the device interrupt to be asserted. 0x0: Device has used a buffer. 0x1 Device config has been changed.
#define VIRTR_INT_ACK_O             0x064   // (W) Writing a value with bits set as defined in `VIRTR_INT_STATUS_O` notifies the device the interrupt has been handled.
#define VIRTR_DEV_STATUS_O          0x070   // (RW) Returns the current device status flags, or writing sets the status flags. Writing 0x0 triggers a device reset.
#define VIRTR_QUEUE_DESC_O          0x080   // (W) Writing to this register notifies the device about location of the descriptor area of the queue selected by writing to `VIRTR_QUEUE_SEL_O`. 
#define VIRTR_QUEUE_DRV_O           0x090   // (W) Writing to this register notifies the device about location of the driver area (avail ring) of the queue selected by writing to `VIRTR_QUEUE_SEL_O`.
#define VIRTR_QUEUE_DEV_O           0x0A0   // (W) Writing to this register notifies the device about location of the device area (used ring) of the queue selected by writing to `VIRTR_QUEUE_SEL_O`.
#define VIRTR_SHM_SEL_O             0x0AC   // (W) Writing to this register selects the shared memory region.
#define VIRTR_SHM_LEN_O             0x0B0   // (R) Returns the length of the selected shared memory region in bytes. Returing from a non-existent region returns -1.
#define VIRTR_SHM_BASE_O            0x0B8   // (R) The driver reads these registers to discover the base address of the region in physical address space. This address is chosen by the device. Reading from non-existant regions returns 0xFFFFFFFFFFFFFFFF.
#define VIRTR_QUEUE_RESET_O         0x0C0   // (RW) If VIRTIO_F_RING_RESET has been negotiated, writing 0x1 to this register selectively resets the queue.
#define VIRTR_CFG_GENERATION_O      0x0FC   // (R) unused in kernel
#define VIRTR_CFG_O                 0x100   // (RW) unused in kernel

#define VIRTIO_MAGIC_NUMBER         0x74726976
#define VIRTIO_VERSION              0x2

#define VIRTIO_STATUS_ACKNOWLEDGE   1
#define VIRTIO_STATUS_DRIVER        2
#define VIRTIO_STATUS_DRIVER_OK     4
#define VIRTIO_STATUS_FEATURES_OK   8
#define VIRTIO_STATUS_NEEDS_RESET   64
#define VIRTIO_STATUS_FAILED        128


// Flag indicating the descriptor is write-only
#define VIRTQ_DESC_F_WRITE (1 << 1)
// Flag indicating the buffer contains another descriptor
#define VIRTA_DESC_F_NEXT (1 << 0)
// Flag indicating that the driver has a stored table of indirect descriptors in memory for use by the device.
#define VIRTQ_DESC_F_INDIRECT (1 << 2)


/*
*   The descriptor area is a structure that holds data about a given buffer. These are intended to be an array
*   within the virtio_queue.
*/
typedef struct {
    uint64_t addr;  // Address of the buffer.
    uint32_t len;   // Length of the buffer.
    uint16_t flags; // 0x0: read only 0x1: Another descriptor is in this buffer. 0x2: Buffer is write only. 0x4: Points to address of other table in memory containing descriptors.
    uint16_t next;  // If set (0x1) the data will continue in another buffer, to create a chain.
} virtq_desc;

// Flag indicating if the device should generate an interrupt on an added buffer.
#define VIRTQ_AVAIL_F_NO_INTERRUPT (1 << 0)

/*
*   A driver write only struct where the driver places descriptors for the device to consume.  
*/
typedef struct {
    uint16_t flags;                     // 0x0: Notify the driver (using interrupts). 0x1: Do not notify the driver.
    uint16_t idx;                       // Indicates where the driver would put the next descriptor entry in the avail ring ( modulo queue_size because it's circular).
    uint16_t ring[/* Queue size */];    // Indexes to descriptors in the descriptor table for the device to consume.
} virtq_avail;

/*
*   A struct containing the used data from the device.
*/
typedef struct {
    uint32_t id;    // The index of start of used descriptor chain (or head of chained descriptors).
    uint32_t len;   // The total length of the descriptor chain which was used (written to) (or total length of all written buffers in a chain).
} virtq_used_elem;

// Flag indicating if the device should notify the driver of a changed buffer.
#define VIRTQ_USED_F_NO_NOTIFY (1 << 0)

/*
*   A device write only struct where the device returns the buffer to the driver.
*/
typedef struct {
    uint16_t flags;                         // 0x0: Notify the driver (using interrupts). 0x1: Do not notify the driver.
    uint16_t idx;                           // Indicates where the driver would put the next descriptor entry in the avail ring ( modulo queue_size because it's circular).
    virtq_used_elem ring[/* Queue size*/];
} virtq_used;

// Quantity of descriptors.
#define VIRTQ_QUEUE_SIZE 128

/*
*   Structure of the Virtual Queue. Must be allocated aligned to 4096 bytes.
*/
typedef struct {
    virtq_desc buffers[VIRTQ_QUEUE_SIZE]; // Descriptors for driver supplied buffers.
    virtq_avail available;          // Metadata for available descriptors (Driver supplied).
    uint8_t padding[4096 - (sizeof(virtq_desc) * VIRTQ_QUEUE_SIZE + sizeof(virtq_avail)) % 4096]; // Align to 4096 bytes
    virtq_used used;                // Metadata for used descriptors (Device supplied).
} virt_queue;

#endif