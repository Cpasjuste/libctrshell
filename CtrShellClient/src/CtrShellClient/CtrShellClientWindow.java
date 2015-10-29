package CtrShellClient;

import java.awt.EventQueue;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.prefs.Preferences;

import javax.swing.JFrame;
import javax.swing.JTextField;
import javax.swing.JTextArea;
import javax.swing.JMenuBar;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.border.EtchedBorder;
import javax.swing.text.Document;
import javax.swing.text.Element;
import javax.swing.JScrollPane;
import java.awt.Color;
import javax.swing.SpringLayout;
import javax.swing.JToolBar;
import javax.swing.JButton;
import javax.swing.ImageIcon;

import java.awt.Dimension;
import java.awt.Font;

public class CtrShellClientWindow {

	private JFrame frame;
	private JTextField textField;
	private JTextArea textArea;
	private JMenuBar menuBar;
	private JMenu mnFile;
	private JMenuItem mntmExit;
	private JMenuItem mntmIpAddress;
	
	private JButton btnConnect;
	private ImageIcon imgConnect;
	private ImageIcon imgDisconnect;
	
	private ConnectionThread console;
	private Socket socket;
	private List<String> history;
	private int historyIndex = 0;
	private Preferences prefs;
	
	private String ctrIpAddress;
	private int connectTimeout = 2000;
	private CtrShellCmdList cmds;
	
	public static boolean isNullOrEmpty(String s) {
	    return s == null || s.length() == 0;
	}
	
	public class ConnectionThread extends Thread {
		
		private boolean running = false;
		private BufferedReader in;
		private PrintWriter out;
        
    	public void run() {
    		
    		running = true;
    		
    		while(running) {
    			if(socket==null) {
    				while(running) {
    					if(Connect(ctrIpAddress, 3333)) {
    						break;
    					}
    				}
    			}
    			
	    		try {
	    			if(in!=null) {
	    				append(""+(char)in.read());
	    			}
				} catch (Exception e) {
					e.printStackTrace();
					Stop();
				}
    		}
    		append("disconnected...\n");
    	}
    	
    	public void Stop() {
    		if(socket==null) {
    			return;
    		}
    		try {sendExit();} catch (Exception e) {}
    		try {socket.shutdownInput();} catch (Exception e) {}
    		try {socket.shutdownOutput();} catch (Exception e) {}
    		try {in.close();} catch (Exception e) {}
    		try {out.close();} catch (Exception e) {}
    		try {socket.close();} catch (Exception e) {}
			socket = null;
    	}
    	
    	public void Kill() {
    		running = false;
    		Stop();
    		try { join(); } catch (Exception e) {}
    	}
    	
    	public boolean Connect(String ip, int port) {
    		
    		Stop();
    		
			try {
				append("Connecting to " + ip+":"+port+" ... ");
				Thread.sleep(connectTimeout/2);
				socket = new Socket();
				socket.connect(new InetSocketAddress(ip, port), connectTimeout/2);
				in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
				out = new PrintWriter(socket.getOutputStream(), true);
				append("connected!\n");
			} catch (Exception e) {
				append("couldn't connect (" + e.getMessage() +")"+ "\n");
				return false;
			}
			return true;
    	}
    	
    	public void send(String[] cmd) {
    		try {
    			debug("cmd:" + cmds.getType(cmd[0]));
    			out.print(cmds.getType(cmd[0]));
    			out.flush();
    			for(int i=1; i<5; i++) {
    				Thread.sleep(100);
    				if(cmd[i]==null) {
    					break;
    				}
    				out.print(cmd[i]);
    				out.flush();
    			}
    		} catch (Exception e) {}
    		try {
    			Thread.sleep(100);
    			out.print("cmdend");
    			out.flush();
		} catch (Exception e2) {};
    	}
    	
    	public void sendFile(boolean exec, String src, String dest) {
    		File fSrc = new File(src);
    		if(!fSrc.exists()) {
    			append("File not found: " + fSrc.getAbsolutePath() + "\n");
    			return;
    		}
    		append("Sending file: " + fSrc.getAbsolutePath() + "\n\n");
    		
    		try {
	            long length = fSrc.length();
	            String cmd = exec ? "title_send" : "put";
	            send( new String[] { cmd, String.valueOf(length), dest } );

	            InputStream ins = new FileInputStream(fSrc);
	            OutputStream outs = socket.getOutputStream();
	            byte buffer[] = new byte[1024 * 64];
	            
	            Thread.sleep(2000);
	            
	            int read = 0;
	            while((read = ins.read(buffer)) != -1) {
	                outs.write(buffer, 0, read);
	            }
	            ins.close();
	            
    		} catch(Exception e) {
    			append("sendfile: " + e.getMessage() + "\n");
    		}
    	}
	}
	
	public void sendExit() {
		console.send( new String[] {"exit"} );
	}
	
	public class CmdHandler implements Runnable {
		
		private String cmd;
		
		public CmdHandler(String cmd) {
			this.cmd = cmd;
		}
		
	    public void run() {
	    	if(cmd.contentEquals("exit")) {
				console.Kill();
				btnConnect.setIcon(imgConnect);
			} else if(cmd.startsWith("title_send")) {
				String[] split = cmd.split(" ");
				if(split.length<2) {
					append("usage: title_send source\n");
					return;
				}
				console.sendFile(true, split[1], null);
			} else if(cmd.startsWith("put")) {
				String[] split = cmd.split(" ");
				if(split.length<3) {
					append("usage: put source destination\n");
					return;
				}
				console.sendFile(false, split[1], split[2]);
			} else {
				String[] split = cmd.split(" ");
				if(cmds.getType(split[0])<1) {
					append("unknow command\n");
					return;
				}
				console.send(split);
			}
	    }
	}
	
	public void append(String s) {
		textArea.append(s);
		textArea.setCaretPosition(textArea.getDocument().getLength());
	}
	
	public void replace(String s) {
		Document document = textArea.getDocument();
		Element rootElem = document.getDefaultRootElement();
		int numLines = rootElem.getElementCount();
		Element lineElem = rootElem.getElement(numLines - 2);
		int lineStart = lineElem.getStartOffset();
		int lineEnd = lineElem.getEndOffset();
		textArea.replaceRange(s, lineStart, lineEnd);
	}
	
	/**
	 * Launch the application.
	 */
	public static void main(String[] args) {
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					final CtrShellClientWindow window = new CtrShellClientWindow();
					
					// init
					window.cmds = new CtrShellCmdList();
					window.prefs = Preferences.userNodeForPackage(this.getClass());
					window.history = new ArrayList<String>();
					window.ctrIpAddress = window.prefs.get("address", "");
					
					window.append(window.cmds.getHelp());
					
					window.frame.setVisible(true);
					window.frame.addWindowListener(new WindowAdapter()
			        {
			            @Override
			            public void windowClosing(WindowEvent e)
			            {
			                debug("Exiting");
			                if(window.console != null) {
			                	window.console.Kill();
							}
			                e.getWindow().dispose();
			            }
			        });	
					
					// Add Listeners
					window.textField.addActionListener(new ActionListener() {
			            public void actionPerformed(ActionEvent e) {
			            	if(window.console != null && window.console.isAlive()) {
			            		//window.buildCmd(window.textField.getText());
			            		Runnable r = window.new CmdHandler(window.textField.getText());
			            		new Thread(r).start();
			            	} else {
			            		window.append("Connect to ctr device first\n");
			            	}
			            	if(!isNullOrEmpty(window.textField.getText())) {
				            	window.history.add(window.textField.getText());
				            	window.historyIndex = window.history.size();
				            	window.textField.setText("");
			            	}
			            }
			        });
					
					window.textField.addKeyListener(new KeyListener() {

						@Override
						public void keyPressed(KeyEvent e) {
							//debug("keyPressed: " + e.getKeyCode());
						}

						@Override
						public void keyReleased(KeyEvent e) {
							//debug("keyReleased: " + e.getKeyCode());
							if(e.getKeyCode()==38) { // KB UP
								if(window.historyIndex>0) {
									window.historyIndex--;
									window.textField.setText(window.history.get(window.historyIndex));
								}
							} else if(e.getKeyCode()==40) { // KB DOWN
								if(window.historyIndex<window.history.size()-1) {
									window.historyIndex++;
									window.textField.setText(window.history.get(window.historyIndex));
								}
							}
						}

						@Override
						public void keyTyped(KeyEvent e) {
							//System.out.println("keyTyped: " + e.getKeyCode());
						}
					});
					
					window.btnConnect.addActionListener(new ActionListener() {
						public void actionPerformed(ActionEvent arg0) {	
							if(isNullOrEmpty(window.ctrIpAddress)) {
								window.setIpAddress();
								window.onButtonConnect();
							} else {
								window.onButtonConnect();
							}
						}
					});
					
					window.mntmIpAddress.addActionListener(new ActionListener(){
						public void actionPerformed(ActionEvent event) {
							window.setIpAddress();
						}
					});
					
					window.textField.requestFocus();
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}

	private void onButtonConnect() {
		if(console == null || !console.isAlive()) {
			if(isNullOrEmpty(this.ctrIpAddress)) {
				return;
			}
			console = new ConnectionThread();
			console.start();
			btnConnect.setIcon(imgDisconnect);
		} else {
			console.Kill();
			btnConnect.setIcon(imgConnect);
		}
	}
	
	private void setIpAddress() {
		String address = JOptionPane.showInputDialog(
	    		null, 
	    		"Enter CTR ip address here ...", 
	    		ctrIpAddress );
	    if(!isNullOrEmpty(address)) {
	    	debug(address);
	    	ctrIpAddress = address;
	    	prefs.put("address", address);
	    }
	}
	
	/**
	 * Create the application.
	 */
	public CtrShellClientWindow() {
		initialize();
	}

	/**
	 * Initialize the contents of the frame.
	 */
	private void initialize() {
		frame = new JFrame();
		frame.setBounds(100, 100, 800, 532);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		SpringLayout springLayout = new SpringLayout();
		frame.getContentPane().setLayout(springLayout);
		
		JToolBar toolBar = new JToolBar();
		springLayout.putConstraint(SpringLayout.NORTH, toolBar, 10, SpringLayout.NORTH, frame.getContentPane());
		springLayout.putConstraint(SpringLayout.SOUTH, toolBar, 59, SpringLayout.NORTH, frame.getContentPane());
		frame.getContentPane().add(toolBar);
		
		textArea = new JTextArea();
		textArea.setFont(new Font("Lucida Console", Font.PLAIN, 13));
		textArea.setForeground(new Color(51, 204, 0));
		textArea.setCaretColor(new Color(0, 204, 0));
		textArea.setBackground(new Color(0, 0, 0));
		textArea.setBorder(new EtchedBorder(EtchedBorder.LOWERED, null, null));
		textArea.setBounds(10, 11, 764, 367);
		textArea.setLineWrap(true);
		textArea.setEditable(false);
		
		JScrollPane scrollPane = new JScrollPane(textArea);
		springLayout.putConstraint(SpringLayout.NORTH, scrollPane, 6, SpringLayout.SOUTH, toolBar);
		springLayout.putConstraint(SpringLayout.WEST, scrollPane, 10, SpringLayout.WEST, frame.getContentPane());
		springLayout.putConstraint(SpringLayout.EAST, scrollPane, -10, SpringLayout.EAST, frame.getContentPane());
		frame.getContentPane().add(scrollPane);
		
		textField = new JTextField();
		springLayout.putConstraint(SpringLayout.SOUTH, scrollPane, -6, SpringLayout.NORTH, textField);
		springLayout.putConstraint(SpringLayout.WEST, toolBar, 0, SpringLayout.WEST, textField);
		springLayout.putConstraint(SpringLayout.EAST, toolBar, 0, SpringLayout.EAST, textField);
		
		imgConnect = new ImageIcon(CtrShellClientWindow.class.getResource("/res/icon_connect.png"));
		imgDisconnect = new ImageIcon(CtrShellClientWindow.class.getResource("/res/icon_disconnect.png"));
		btnConnect = new JButton("");
		btnConnect.setToolTipText("Connect");
		btnConnect.setMinimumSize(new Dimension(48, 48));
		btnConnect.setMaximumSize(new Dimension(48, 48));
		btnConnect.setIcon(imgConnect);
		
		toolBar.add(btnConnect);
		
		springLayout.putConstraint(SpringLayout.NORTH, textField, -30, SpringLayout.SOUTH, frame.getContentPane());
		springLayout.putConstraint(SpringLayout.WEST, textField, 10, SpringLayout.WEST, frame.getContentPane());
		springLayout.putConstraint(SpringLayout.SOUTH, textField, -10, SpringLayout.SOUTH, frame.getContentPane());
		springLayout.putConstraint(SpringLayout.EAST, textField, -10, SpringLayout.EAST, frame.getContentPane());
		textField.setBorder(new EtchedBorder(EtchedBorder.LOWERED, null, null));
		frame.getContentPane().add(textField);
		textField.setColumns(10);
		textField.setFocusTraversalKeysEnabled(false);
		
		menuBar = new JMenuBar();
		frame.setJMenuBar(menuBar);
		
		mnFile = new JMenu("File");
		menuBar.add(mnFile);
		
		mntmExit = new JMenuItem("Exit");
		mnFile.add(mntmExit);
		
		JMenu mnSettings = new JMenu("Settings");
		menuBar.add(mnSettings);
		
		mntmIpAddress = new JMenuItem("ip address");
		mnSettings.add(mntmIpAddress);
	}
	
	static void debug(String s) {
		System.out.println(s);
	}
}
