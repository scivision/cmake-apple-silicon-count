// ASPL license: https://opensource.apple.com/license/apsl/
//------------------------------------------------------------
// build like:
//   clang hw.c -framework Foundation -framework IOKit
// ./a.out then gives with an M1 Apple Silicon CPU:
//
// cpu0@0(E): apple,icestorm
// cpu1@0(E): apple,icestorm
// cpu2@0(E): apple,icestorm
// cpu3@0(E): apple,icestorm
// cpu4@1(P): apple,firestorm
// cpu5@1(P): apple,firestorm
// cpu6@1(P): apple,firestorm
// cpu7@1(P): apple,firestorm

// code by @fozog  https://github.com/open-mpi/hwloc/issues/454
// cluster types: https://developer.apple.com/news/?id=vk3m204o
// E=Efficiency, P=Performance
// derived from https://opensource.apple.com/source/IOKitTools/IOKitTools-56/ioreg.tproj/ioreg.c.auto.html

#include <stdio.h>
#include <IOKit/IOKitLib.h>                           // (IOMasterPort, ...)
#include <CoreFoundation/CoreFoundation.h>

#define DT_PLANE "IODeviceTree"

static void assertion(int condition, char * message)
{
    if (condition == 0)
    {
        fprintf(stderr, "ioreg: error: %s.\n", message);
        exit(1);
    }
}

struct cpu {
    int cluster;
    char cluster_type;
    int logical_id;
    char* compatible;
};

static char* get_first_string(CFDataRef object)
{
    const UInt8 * bytes;
    CFIndex       index;
    CFIndex       length;

    length = CFDataGetLength(object);
    bytes  = CFDataGetBytePtr(object);

    for (index = 0; index < length; index++)  // (scan for ascii string/strings)
    {
        if (bytes[index] == 0)
        {
            // can return a zero length string '\0'
            char* value = malloc(index+1);
            if (value != NULL) strncpy(value, (char*)bytes, index+1);
            return value;
        }
    }

    return NULL;

}

static char* get_string(CFStringRef object)
{
    const char * c = CFStringGetCStringPtr(object, kCFStringEncodingMacRoman);

    if (c)
        return strdup(c);
    else
    {
        CFIndex bufferSize = CFStringGetLength(object) + 1;
        char *  buffer     = malloc(bufferSize);

        if (buffer)
        {
            if ( CFStringGetCString(
                    /* string     */ object,
                    /* buffer     */ buffer,
                    /* bufferSize */ bufferSize,
                    /* encoding   */ kCFStringEncodingMacRoman ) )


                return buffer;
        }
        return NULL;
    }
}

static int get_number(CFNumberRef object, long long* result)
{
    return CFNumberGetValue(object, kCFNumberLongLongType, result);
}

static void get_cpu(const void * key, const void * value, void * parameter)
{
    struct cpu* cpu = (struct cpu*)parameter;
    char* name = get_string(key);
    if (strcmp(name, "compatible") == 0) {
        cpu->compatible = get_first_string(value);
    }
    else if (strcmp(name, "logical-cluster-id") == 0) {
        long long number = -1;
        if (get_number(value, &number))
             cpu->cluster = (int)number;
    }
    else if (strcmp(name, "logical-cpu-id") == 0) {
        long long number = -1;
        if (get_number(value, &number))
             cpu->logical_id = (int)number;
    }
    else if (strcmp(name, "cluster-type") == 0) {
        char* type = get_first_string(value);
        if (type != NULL && strlen(type)==1)
            cpu->cluster_type = type[0];
        else
            cpu->cluster_type = '?';
        free(type);
    }
    free(name);
}

bool has_cpu_started = false;
bool last_cpu_seen = false;

static void scan( io_registry_entry_t service,
                  Boolean             serviceHasMoreSiblings,
                  UInt32              serviceDepth,
                  UInt64              stackOfBits )
{
    io_registry_entry_t child       = 0; // (needs release)
    io_registry_entry_t childUpNext = 0; // (don't release)
    io_iterator_t       children    = 0; // (needs release)
    kern_return_t       status      = KERN_SUCCESS;

    io_name_t       name;           // (don't release)

    // Obtain the service's children.

    status = IORegistryEntryGetChildIterator(service, DT_PLANE, &children);
    assertion(status == KERN_SUCCESS, "can't obtain children");

    childUpNext = IOIteratorNext(children);

    // Save has-more-siblings state into stackOfBits for this depth.

    if (serviceHasMoreSiblings)
        stackOfBits |=  (1 << serviceDepth);
    else
        stackOfBits &= ~(1 << serviceDepth);

    // Save has-children state into stackOfBits for this depth.

    if (childUpNext)
        stackOfBits |=  (2 << serviceDepth);
    else
        stackOfBits &= ~(2 << serviceDepth);

    // Print out the relevant service information.
    status = IORegistryEntryGetNameInPlane(service, DT_PLANE, name);
    assertion(status == KERN_SUCCESS, "can't obtain name");

    // Traverse over the children of this service.
    if (has_cpu_started)
    {
            // handle cpu
        CFMutableDictionaryRef properties = 0; // (needs release)
        struct cpu cpu;

        status = IORegistryEntryCreateCFProperties(service,
                                                   &properties,
                                                   kCFAllocatorDefault,
                                                   kNilOptions);
        assertion(status == KERN_SUCCESS, "can't obtain properties");
        assertion(CFGetTypeID(properties) == CFDictionaryGetTypeID(), NULL);

        CFDictionaryApplyFunction(properties, get_cpu, &cpu);

        printf("%s@%d(%c): %s\n", name, cpu.cluster, cpu.cluster_type, cpu.compatible);

        CFRelease(properties);

    }
    else if (strcmp(name, "cpus") == 0)
    {
        has_cpu_started = true;
    }

    while (childUpNext && !last_cpu_seen)
    {
        child       = childUpNext;
        childUpNext = IOIteratorNext(children);

        scan( /* service                */ child,
              /* serviceHasMoreSiblings */ (childUpNext) ? TRUE : FALSE,
              /* serviceDepth           */ serviceDepth + 1,
              /* stackOfBits            */ stackOfBits);

        IOObjectRelease(child); child = 0;
    }

    IOObjectRelease(children); children = 0;

    if (has_cpu_started && strcmp(name, "cpus") == 0)
    {
        last_cpu_seen = true;
    }
}

int main(int argc, const char * argv[]) {

    io_registry_entry_t service   = 0; // (needs release)

    service = IORegistryGetRootEntry(kIOMasterPortDefault);
    assertion(service, "can't obtain I/O Kit's root service");

    scan( /* service                */ service,
          /* serviceHasMoreSiblings */ FALSE,
          /* serviceDepth           */ 0,
          /* stackOfBits            */ 0
         );

    IOObjectRelease(service); service = 0;

    return 0;

}
