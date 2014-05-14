#ifndef LD_CHECKER_H
#define LD_CHECKER_H

#include <stdio.h>
#include <string.h>

#define PRINT_KERNEL_BUFFER_CREATION 0
#define PRINT_BUFFER_DIRECTION 0
#define PRINT_BUFFER_TRANSFER 0
#define PRINT_BUFFER_RELEASE 0

#define PRINT_KERNEL_BEFORE_EXEC 1
#define PRINT_KERNEL_AFTER_EXEC 1
#define PRINT_KERNEL_AFTER_EXEC_IGNORE_CONST 0
#define PRINT_KERNEL_NAME_ONLY 0

#define PRINT_KERNEL_ARG_FULL_BUFFER 1
#define FULL_BUFFER_SIZE_LIMIT 750
#define PRINT_FULL_PARAMS_TO_FILE 1

#define PRINT_BUFFER_TRANSFER_FIRST_BYTES_AS_FLOAT 0

#define BUFFER_ZERO_IS_NULL 1

#define FORCE_FINISH_KERNEL 0

#define WITH_MPI 0
#define ONLY_MPI_ROOT_OUTPUT 1

typedef int ld_flags;

#define LD_FLAG_READ_ONLY 1
#define LD_FLAG_WRITE_ONLY 2
#define LD_FLAG_READ_WRITE 4

struct ld_bindings_s {
  const char *name;
  void **real_address;
};

enum type_info_type_e {
  TYPE_INFO_DEFAULT,
  TYPE_INFO_FLOAT,
  TYPE_INFO_DOUBLE,
  TYPE_INFO_INT,
  TYPE_INFO_UINT
};

extern struct type_info_s {
  const char *type_name;
  const char *format;
  const unsigned int size;
  const enum type_info_type_e type;
} TYPE_FORMATTERS[] ;

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

#define FIRST_BYTES_TO_READ 32
struct ld_mem_s {
  void *handle;
  unsigned int uid;
  size_t size;
  ld_flags flags;
  int has_values;
  int values_outdated;
  int released;
  char first_values[FIRST_BYTES_TO_READ];
};

#define CURRENT_VALUE_BUFFER_SZ 80
struct ld_kern_param_s {
  const char *name;
  const char *type;
  const struct type_info_s *type_info;
  int is_pointer;
  int has_current_value;
  struct ld_mem_s *current_buffer;
  size_t offset;
  
  char current_value[CURRENT_VALUE_BUFFER_SZ];
  char current_binary_value[10];
};

struct ld_kernel_s {
  void *handle;
  int nb_params;
  const char *name;
  struct ld_kern_param_s *params;
  unsigned int exec_counter;
};

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

void dbg_crash_event(void);
void dbg_notify_event(void);

struct callback_s {
  int (*getBufferContent) (struct ld_mem_s *ldBuffer, void *buffer,
                           size_t offset, size_t size);
};

void debug(const char *format, ...);
void info(const char *format, ...);
void warning(const char *format, ...);
void error(const char *format, ...);
void gpu_trace(const char *format, ...);

/* 
 * struct ld_name_s *find_name_entry(cl_key_type key);
 * struct ld_name_s *get_next_name_spot();
 * int add_to_name_map (cl_key_type key);
 * struct ld_name_s *get_name_entry(int id));
 *
 * */

#define assert(val) {if (!val) {error ("pointer " #val " is null at %s:%d %s\n", \
                                       __FILE__, __LINE__,  __func__);}}

#define CREATE_HASHMAP(_NAME, _KEY_TYPE,_MAX_KEYS)                      \
  struct ld_##_NAME##_s _NAME##_map[_MAX_KEYS];                         \
  unsigned int _NAME##_elt_count = 0;                                   \
                                                                        \
  int find_##_NAME##_entry_id (_KEY_TYPE key);                          \
  struct ld_##_NAME##_s *get_##_NAME##_entry (int id);                  \
  struct ld_##_NAME##_s *find_##_NAME##_entry(_KEY_TYPE key);           \
  struct ld_##_NAME##_s *get_next_##_NAME##_spot(void);                 \
  int add_to_##_NAME##_map (_KEY_TYPE key);                             \
                                                                        \
  int find_##_NAME##_entry_id (_KEY_TYPE key) {                         \
    int i;                                                              \
    for (i = 0; i < _NAME##_elt_count; i++) {                           \
      if (_NAME##_map[i].handle == key) {                               \
        return i ;                                                      \
      }                                                                 \
    }                                                                   \
    return -1;                                                          \
  }                                                                     \
                                                                        \
  inline struct ld_##_NAME##_s *get_##_NAME##_entry (int id) {          \
    return (id < _NAME##_elt_count && id >= 0 ?                         \
            &_NAME##_map[id] : NULL);                                   \
  }                                                                     \
                                                                        \
  inline struct ld_##_NAME##_s *find_##_NAME##_entry(_KEY_TYPE key) {   \
    return get_##_NAME##_entry(find_##_NAME##_entry_id (key));          \
  }                                                                     \
                                                                        \
  inline struct ld_##_NAME##_s *get_next_##_NAME##_spot(void) {         \
    if (_NAME##_elt_count >= _MAX_KEYS) {                               \
      return NULL;                                                      \
    }                                                                   \
                                                                        \
    return &_NAME##_map[_NAME##_elt_count++];                           \
  }                                                                     \
                                                                        \
  inline int add_to_##_NAME##_map (_KEY_TYPE key) {                    \
    struct ld_##_NAME##_s *next;                                        \
      next = get_next_##_NAME##_spot();                                 \
        if (!next) {                                                    \
          return -1;                                                    \
        }                                                               \
        next->handle = key;                                             \
        return _NAME##_elt_count - 1;   /* next - start / size ? */     \
  }     

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

static inline char *buffer_flag_to_string(ld_flags flags) {
  switch(flags) {
  case LD_FLAG_READ_ONLY: return "READ_WRITE";
  case LD_FLAG_WRITE_ONLY: return "WRITE_ONLY";
  case LD_FLAG_READ_WRITE: return "READ_WRITE";
  default: return "UNKNWON";
  }
}

static inline char *buffer_flag_to_direction(ld_flags flags) {
#if  PRINT_BUFFER_DIRECTION
  switch(flags) {
  case LD_FLAG_READ_ONLY: return "--->";
  case LD_FLAG_WRITE_ONLY: return "<---";
  case LD_FLAG_READ_WRITE: return "<-->";
  default: return "?--?";
  }
#else
  return "";
#endif
}

struct work_size_s {
  size_t local[3];
  size_t global[3];
};

void init_ldchecker(struct callback_s callbacks, struct ld_bindings_s *lib_bindings);
void buffer_created_event(struct ld_mem_s *buffer);
void subbuffer_created_event(struct ld_mem_s *buffer, size_t offset);
#define LD_WRITE 0
#define LD_READ 1
void buffer_copy_event(struct ld_mem_s *buffer, int is_read, void **ptr,
                       size_t size, size_t offset);
void buffer_released (struct ld_mem_s *ldBuffer);
void kernel_created_event(struct ld_kernel_s *kernel);
void kernel_set_scalar_arg_event (struct ld_kernel_s *ldKernel,
                                  struct ld_kern_param_s *ldParam,
                                  int arg_index,
                                  const void **arg_value);
void kernel_set_buffer_arg_event (struct ld_kernel_s *ldKernel,
                                  struct ld_kern_param_s *ldParam,
                                  int arg_index,
                                  struct ld_mem_s *ldBuffer,
                                  size_t offset);
void kernel_executed_event(struct ld_kernel_s *kernel,
                           const struct work_size_s *work_sizes,
                           int work_dim);
void kernel_finished_event(struct ld_kernel_s *ldKernel,
                           const struct work_size_s *work_sizes,
                           int work_dim);
/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

static inline const struct type_info_s *get_type_info (const char *type_name) {
  int i;

  for (i = 0; TYPE_FORMATTERS[i].type_name
         && (!type_name || !strstr(type_name, TYPE_FORMATTERS[i].type_name)); i++);

  return &TYPE_FORMATTERS[i];
}

static inline int is_pointer_type (const char *type_name) {
  int i;

  if (!type_name) {
    return 0;
  }
  
  for(i = 0; type_name[i]; i++) {
    if (type_name[i] == '*') {
      return 1;
    }
  }
  return 0;
}

#endif

