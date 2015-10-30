"libctrshell" is a library to add to your (cia) project, and will provide a shell over wifi to print debug messages over the network and a few more functions.

I never made it public as it may contains a lot of bugs, is not finished and the code is crappy but it still improved my development time...

I won't go too deep in the details as i'm too lazy for this now, but for short you should :

- build libctrshell and add it to your (cia) project
- use "ctr_shell_init(NULL, SHELL_PORT)" to start the server in your project
- use "ctr_shell_print" to printf to the client
- build the java client or download it to connect to your 3ds/project
- use "title_send [cia_path] to send/exec your newly compiled project to the 3ds

Source code : https://github.com/Cpasjuste/libctrshell

Here is a list of the available cmds :
[code]
[LIST=1]
[*]  ls [path] - list directory content
[*]  cd [path] - enter directory
[*]  rm [file] - delete file
[*]  rmdir [dir] - delete directory
[*]  mkdir [dir] - create directory
[*]  mv [path] [newPath] - move file/directory
[*]  pwd - echo current directory
[*]  title_list [0/1] - list titles (0=nand/1=sd)
[*]  title_info [file/titleid] - get cia/title infos
[*]  title_del [titleid] - delete given titleid
[*]  title_install [ciaFile] - install cia
[*]  title_exec [titleid] - execute given titleid
[*]  title_send [localCia] - send, install and execute cia file
[*]  put [localFile] [remoteFile] - send file to ctr sdcard
[*]  memr [address] [lenght] - read u32 block of memory (memr 0x1FF810A0 1)
[*]  memw [address] [u32] - write u32 block to memory (memw 0x1FF810A0 00000000)
[*]  menu - return to menu
[*]  reset - reload CtrShell
[*]  exit - disconnect from CtrShell
[*]  help - show this help
[/LIST]
[/code]
