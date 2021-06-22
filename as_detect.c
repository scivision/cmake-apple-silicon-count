// by @bgoglin: https://github.com/open-mpi/hwloc/issues/454#issuecomment-864402498
// BSD license

#include <stdio.h>
#include <stdlib.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

#define DT_PLANE "IODeviceTree"

int main()
{
  io_registry_entry_t cpus_root;
  io_iterator_t cpus_iter;
  io_registry_entry_t cpus_child;
  kern_return_t kret;

  cpus_root = IORegistryEntryFromPath(kIOMasterPortDefault, DT_PLANE ":/cpus");
  if (!cpus_root) {
    fprintf(stderr, "failed to find IODeviceTree:/cpus\n");
    exit(EXIT_FAILURE);
  }

  kret = IORegistryEntryGetChildIterator(cpus_root, DT_PLANE, &cpus_iter);
  if (kret != KERN_SUCCESS) {
    fprintf(stderr, "failed to create iterator\n");
    exit(EXIT_FAILURE);
  }

  while ((cpus_child = IOIteratorNext(cpus_iter)) != 0) {
    io_name_t name;
    CFTypeRef ref;

    kret = IORegistryEntryGetNameInPlane(cpus_child, DT_PLANE, name);
    if (kret != KERN_SUCCESS)
      continue;
    printf("looking at %s\n", name);

    ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE, CFSTR("logical-cpu-id"), kCFAllocatorDefault, kNilOptions);
    if (!ref) {
      fprintf(stderr, "failed to find logical-cpu-id\n");
    } else {
      printf("logical-cpu-id type is: %s\n",
             CFStringGetCStringPtr(CFCopyTypeIDDescription(CFGetTypeID(ref)), kCFStringEncodingUTF8));
      if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
        long long value;
        if (CFNumberGetValue(ref, kCFNumberLongLongType, &value))
          printf("got logical-cpu-id %lld\n", value);
        else
          printf("failed to get logical-cpu-id\n");
      }
    }

    ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE, CFSTR("logical-cluster-id"), kCFAllocatorDefault, kNilOptions);
    if (!ref) {
      fprintf(stderr, "failed to find logical-cluster-id\n");
    } else {
      printf("logical-cluster-id type is: %s\n",
             CFStringGetCStringPtr(CFCopyTypeIDDescription(CFGetTypeID(ref)), kCFStringEncodingUTF8));
      if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
        long long value;
        if (CFNumberGetValue(ref, kCFNumberLongLongType, &value))
          printf("got logical-cluster-id %lld\n", value);
        else
          printf("failed to get logical-cluster-id\n");
      }
      CFRelease(ref);
    }

    ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE, CFSTR("cluster-type"), kCFAllocatorDefault, kNilOptions);
    if (!ref) {
      fprintf(stderr, "failed to find cluster-type\n");
    } else {
      printf("cluster-type type is: %s\n",
             CFStringGetCStringPtr(CFCopyTypeIDDescription(CFGetTypeID(ref)), kCFStringEncodingUTF8));
      if (CFGetTypeID(ref) == CFDataGetTypeID()) {
        if (CFDataGetLength(ref) >= 2) {
          UInt8 value[2];
          CFDataGetBytes(ref, CFRangeMake(0, 2), value);
          if (value[1] == 0)
            printf("got cluster-type %c\n", value[0]);
          else
            printf("got more than one character in cluster-type data %c%c...\n", value[0], value[1]);
        } else
          printf("got only %ld bytes in cluster-type data \n", CFDataGetLength(ref));
      }
      CFRelease(ref);
    }

    ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE, CFSTR("compatible"), kCFAllocatorDefault, kNilOptions);
    if (!ref) {
      fprintf(stderr, "failed to find compatible\n");
    } else {
      printf("compatible type is: %s\n",
             CFStringGetCStringPtr(CFCopyTypeIDDescription(CFGetTypeID(ref)), kCFStringEncodingUTF8));
      if (CFGetTypeID(ref) == CFDataGetTypeID()) {
#define HWLOC_DARWIN_COMPATIBLE_MAX 64
        UInt8 value[HWLOC_DARWIN_COMPATIBLE_MAX+1];
        value[HWLOC_DARWIN_COMPATIBLE_MAX] = 0;
        CFDataGetBytes(ref, CFRangeMake(0, HWLOC_DARWIN_COMPATIBLE_MAX), value);
        if (value[0])
          printf("got compatible %s\n", value);
        else
          printf("compatible is empty\n");
      }
      CFRelease(ref);
    }

    ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE, CFSTR("name"), kCFAllocatorDefault, kNilOptions);
    if (!ref) {
      fprintf(stderr, "failed to find name\n");
    } else {
      printf("name type is: %s\n",
             CFStringGetCStringPtr(CFCopyTypeIDDescription(CFGetTypeID(ref)), kCFStringEncodingUTF8));
      if (CFGetTypeID(ref) == CFDataGetTypeID()) {
#define HWLOC_DARWIN_NAME_MAX 64
        UInt8 value[HWLOC_DARWIN_NAME_MAX+1];
        value[HWLOC_DARWIN_NAME_MAX] = 0;
        CFDataGetBytes(ref, CFRangeMake(0, HWLOC_DARWIN_NAME_MAX), value);
        if (value[0])
          printf("got name %s\n", value);
        else
          printf("name is empty\n");
      }
      CFRelease(ref);
    }

    IOObjectRelease(cpus_child);
  }
  IOObjectRelease(cpus_iter);
  IOObjectRelease(cpus_root);

  exit(EXIT_SUCCESS);
}
