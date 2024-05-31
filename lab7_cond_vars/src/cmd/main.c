#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct record {
  char name[80];
  char address[80];
  uint8_t semester;
};

int fileDescriptor = -1;

const char *record2str(struct record *record) {
  static char buffer[] =
      "{ "
      "\"_________1_________2_________3_________4_________5_________6_________7_________8\" "
      "\"_________1_________2_________3_________4_________5_________6_________7_________8\" "
      "Semester 256"
      " }";
  sprintf(
      buffer,
      "{ \"%s\" \"%s\" Semester %hhu }",
      record->name,
      record->address,
      record->semester
  );
  return buffer;
}

int originIndex = -1;
struct record origin;
struct record target;
int recordCount = 0;
struct record *records = NULL;

void mapRecords() {
  struct stat stat;
  if (fstat(fileDescriptor, &stat) == -1) {
    perror("Could not get stat of file\n");
    return;
  }
  records = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fileDescriptor, 0);
  recordCount = stat.st_size / sizeof(*records);
}

void mapRecordsForChange() {
  struct stat stat;
  if (fstat(fileDescriptor, &stat) == -1) {
    perror("Could not get stat of file\n");
    return;
  }
  records = mmap(
      NULL,
      stat.st_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fileDescriptor,
      0
  );
  recordCount = stat.st_size / sizeof(*records);
}

void unmapRecords() {
  if (records == NULL) return;
  struct stat stat;
  if (fstat(fileDescriptor, &stat) == -1)
    perror("Could not get stat of file\n");
  munmap(records, stat.st_size);
  recordCount = 0;
  records = NULL;
}

void commandList() {
  mapRecords();
  int numberColumns = ceil(log10(recordCount - 1));
  for (int i = 0; i < recordCount; i++) {
    printf("#%*d %s\n", numberColumns, i, record2str(&records[i]));
  }
  unmapRecords();
}

void commandRead(int index) {
  mapRecords();
  if (!(0 <= index && index < recordCount)) {
    printf("Index %d out of range [%d;%d)\n", index, 0, recordCount);
    unmapRecords();
    return;
  }
  originIndex = index;
  target = origin = records[index];
  unmapRecords();
}

char *rtrim(char *s) {
  while (isspace(*s))
    s++;
  return s;
}

char *ltrim(char *s) {
  char *back = s + strlen(s) - 1;
  while (isspace(*back))
    back--;
  back[1] = 0;
  return s;
}

char *trim(char *s) { return rtrim(ltrim(s)); }

void commandUpdate() {
  if (originIndex == -1) {
    printf("Nothing was loaded\n");
    return;
  }
  char buffer[80] = {0};
  char *trimmed = NULL;

  printf("Enter new name [%s]: ", target.name);
  fgets(buffer, sizeof(buffer), stdin);
  trimmed = trim(buffer);
  if (strlen(trimmed) > 0) strcpy(target.name, trimmed);

  printf("Enter new address [%s]: ", target.address);
  fgets(buffer, sizeof(buffer), stdin);
  trimmed = trim(buffer);
  if (strlen(trimmed) > 0) strcpy(target.address, trimmed);

  printf("Enter new semester [%d]: ", target.semester);
  fgets(buffer, sizeof(buffer), stdin);
  sscanf(buffer, " %hhu", &target.semester);
}

void commandSave() {
  if (originIndex == -1) {
    printf("Nothing was loaded\n");
    return;
  }
  struct flock lock = {
      .l_pid = getpid(),
      .l_start = originIndex * sizeof(struct record),
      .l_len = sizeof(struct record),
      .l_type = F_WRLCK,
      .l_whence = SEEK_SET,
  };
  fcntl(fileDescriptor, F_SETLK, &lock);
  lock.l_type = F_UNLCK;
  mapRecordsForChange();
  if (memcmp(&origin, &records[originIndex], sizeof(origin)) != 0) {
    unmapRecords();
    fcntl(fileDescriptor, F_SETLK, &lock);
    printf("Record was changed in file. Abort saving. Record will be reloaded\n"
    );
    commandRead(originIndex);
    return;
  }
  records[originIndex] = target;
  unmapRecords();
  fcntl(fileDescriptor, F_SETLK, &lock);
  originIndex = -1;
}

void handleCommand(const char *line) {
  int index;
  if (line == NULL || strcmp(line, "\n") == 0) {
    // nothing
  } else if (strcmp(line, "LST\n") == 0) {
    commandList();
  } else if (strcmp(line, "UPDATE\n") == 0) {
    commandUpdate();
  } else if (strcmp(line, "PUT\n") == 0) {
    commandSave();
  } else if (sscanf(line, "GET %d\n", &index) == 1) {
    commandRead(index);
  } else {
    printf("Unknown command: %s", line);
  }
}

int main() {
  fileDescriptor = open("build/data.bin", O_RDWR, S_IRUSR | S_IWUSR);
  while (!feof(stdin)) {
    char buffer[256] = {0};
    printf(">>> ");
    handleCommand(fgets(buffer, sizeof(buffer), stdin));
  }
  close(fileDescriptor);
}
