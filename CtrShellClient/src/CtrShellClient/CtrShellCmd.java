package CtrShellClient;

public class CtrShellCmd {
	
	private String name;
	private String[] args;
	private String help;

	public CtrShellCmd(String n, String[] a, String h) {
		name = n;
		args = a;
		help = h;
	}
	
	public String getName() {
		return this.name;
	}
	
	public String[] getArgs() {
		return args;
	}
	
	public int getArgsCount() {
		return args.length;
	}
	
	@Override
	public String toString() {
		String ret = name;
		for(int i=0; i<args.length; i++) {
			ret += " " + args[i];
		}
		ret += " - " + help;
		return ret;
	}
}

