#ifndef _shell_task_h_
#define _shell_task_h_

#ifdef __cplusplus
extern "C" {
#endif

int handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);
int shell_echo_get(void);
void shell_echo_set(int en_flag);
void shell_task(void *pvParameters);
int shell_cmd_forward(char *cmd, u16 cmd_len);

#ifdef __cplusplus
}
#endif

#endif /* _shell_task_h_ */