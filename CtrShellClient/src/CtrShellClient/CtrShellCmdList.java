package CtrShellClient;

import java.util.ArrayList;
import java.util.List;

public class CtrShellCmdList {
	
	public List<CtrShellCmd> cmds;
	
	public CtrShellCmdList() {
		
		cmds = new ArrayList<CtrShellCmd>();
		
		cmds.add( new CtrShellCmd("none", new String[] {}, "none") );
		cmds.add( new CtrShellCmd("ls", new String[] {"[path]"}, "list directory content") );
		cmds.add( new CtrShellCmd("cd", new String[] {"[path]"}, "enter directory") );
		cmds.add( new CtrShellCmd("rm", new String[] {"[file]"}, "delete file") );
		cmds.add( new CtrShellCmd("rmdir", new String[] {"[dir]"}, "delete directory") );
		cmds.add( new CtrShellCmd("mkdir", new String[] {"[dir]"}, "create directory") );
		cmds.add( new CtrShellCmd("mv", new String[] {"[path]", "[newPath]"}, "move file/directory") );
		cmds.add( new CtrShellCmd("pwd", new String[] {}, "echo current directory") );
		cmds.add( new CtrShellCmd("title_list", new String[] {"[0/1]"}, "list titles (0=nand/1=sd)") );
		cmds.add( new CtrShellCmd("title_info", new String[] {"[file/titleid]"}, "get cia/title infos") );
		cmds.add( new CtrShellCmd("title_del", new String[] {"[titleid]"}, "delete given titleid") );
		cmds.add( new CtrShellCmd("title_install", new String[] {"[ciaFile]"}, "install cia") );
		cmds.add( new CtrShellCmd("title_exec", new String[] {"[titleid]"}, "execute given titleid") );
		cmds.add( new CtrShellCmd("title_send", new String[] {"[localCia]"}, "send, install and execute cia file") );
		cmds.add( new CtrShellCmd("put", new String[] {"[localFile]", "[remoteFile]"}, "send file to ctr sdcard") );
		cmds.add( new CtrShellCmd("memr", new String[] {"[address]", "[lenght]"}, "read u32 block of memory (memr 0x1FF810A0 1)") );
		cmds.add( new CtrShellCmd("memw", new String[] {"[address]", "[u32]"}, "write u32 block to memory (memw 0x1FF810A0 00000000)") );
		cmds.add( new CtrShellCmd("menu", new String[] {}, "return to menu") );
		cmds.add( new CtrShellCmd("reset", new String[] {}, "reload CtrShell") );
		cmds.add( new CtrShellCmd("exit", new String[] {}, "disconnect from CtrShell") );
		cmds.add( new CtrShellCmd("help", new String[] {}, "show this help") );
	}
	
	public int getType(String type) {
		for(int i=0; i<cmds.size(); i++) {
			if(cmds.get(i).getName().equals(type)) {
				return i;
			}
		}
		return 0;
	}
	
	public String getName(int type) {
		if(type<cmds.size()) {
			return cmds.get(type).getName();
		}
		return "none";
	}
	
	public String getHelp() {
		String help = "=============\n";
		help += "Available commands: \n";
		help += "=============\n";
		for(int i=1; i<cmds.size(); i++) {
			help += "  " + cmds.get(i).toString() + "\n";
		}
		help += "\n";
		return help;
	}
	
	public boolean contains(String cmd) {
		for(int i=0; i<cmds.size(); i++) {
			if(cmds.get(i).getName().equals(cmd)) {
				return true;
			}
		}
		return false;
	}
}
