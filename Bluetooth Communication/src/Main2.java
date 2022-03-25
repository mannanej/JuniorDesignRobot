import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.Scanner;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JPanel;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import com.fazecast.jSerialComm.SerialPort;

public class Main2 {

	static SerialPort chosenPort;
	static int x = 0;

	public static void main(String[] args) {

		// create and configure the window
		JFrame window = new JFrame();
		window.setTitle("Sensor Graph GUI");
		window.setSize(600, 400);
		window.setLayout(new BorderLayout());
		window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// create a drop-down box and connect button, then place them at the top of the
		// window
		JComboBox<String> portList = new JComboBox<String>();
		JButton startButton = new JButton("Start");
		JPanel topPanel = new JPanel();
		topPanel.add(portList);
		topPanel.add(startButton);
		window.add(topPanel, BorderLayout.NORTH);

		// populate the drop-down box
		SerialPort[] portNames = SerialPort.getCommPorts();
		for (int i = 0; i < portNames.length; i++) {
			portList.addItem(portNames[i].getSystemPortName());
		}

		// create the line graph
		XYSeries series = new XYSeries("Current");
		XYSeriesCollection dataset = new XYSeriesCollection(series);
		JFreeChart chart = ChartFactory.createXYLineChart("Current Readings", "Time (seconds)", "Current (mA)",
				dataset);
		window.add(new ChartPanel(chart), BorderLayout.CENTER);

		// configure the connect button and use another thread to listen for data
		startButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				if (startButton.getText().equals("Start")) {
					// attempt to connect to the serial port
					chosenPort = SerialPort.getCommPort(portList.getSelectedItem().toString());
					chosenPort.setComPortParameters(115200, 8, 1, SerialPort.NO_PARITY);
					chosenPort.setComPortTimeouts(SerialPort.TIMEOUT_SCANNER, 0, 0);
					while (true) {
						if (chosenPort.openPort()) {
							System.out.println("Port is Open");
							startButton.setText("Disconnect");
							portList.setEnabled(false);
//							try {
//								TimeUnit.SECONDS.wait(10);
//							} catch (InterruptedException e) {
//								e.printStackTrace();
//							}
							while (true) {
								try {
									if (System.in.read() == 83) { // If the console gets an "S"
										System.out.println("Got an S");
										byte[] sendS = new byte[] { (byte) 0x53 };
										chosenPort.writeBytes(sendS, 1);
										break;
									}
								} catch (IOException e) {
									e.printStackTrace();
								}
							}
							break;
						} else {
							// Wait until open
						}
					}

					// create a new thread that listens for incoming text and populates the graph
					Thread thread = new Thread() {
						@Override
						public void run() {
							System.out.println("Made it to Run");
							byte[] sendEnter = new byte[] { (byte) 0x0A };
							chosenPort.writeBytes(sendEnter, 1);
							Scanner scanner = new Scanner(chosenPort.getInputStream());
							while (scanner.hasNextLine()) {
								System.out.println("Has Next Line");
								try {
									String line = scanner.nextLine();
									System.out.println(line);
									// int number = Integer.parseInt(line);
									// series.add(x++, 1023 - number);
									series.add(x++, 1023 - (Math.random() * 10));
									window.repaint();
								} catch (Exception e) {
									e.printStackTrace();
								}
							}
							System.out.println("Closing");
							scanner.close();
						}
					};
					thread.start();
				} else {
					// disconnect from the serial port
					chosenPort.closePort();
					portList.setEnabled(true);
					startButton.setText("Start");
					series.clear();
					x = 0;
				}
			}
		});
		// show the window
		window.setVisible(true);
	}
}
