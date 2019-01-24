import static org.monte.media.FormatKeys.EncodingKey;
import static org.monte.media.FormatKeys.FrameRateKey;
import static org.monte.media.FormatKeys.KeyFrameIntervalKey;
import static org.monte.media.FormatKeys.MIME_AVI;
import static org.monte.media.FormatKeys.MediaTypeKey;
import static org.monte.media.FormatKeys.MimeTypeKey;
import static org.monte.media.VideoFormatKeys.CompressorNameKey;
import static org.monte.media.VideoFormatKeys.DepthKey;
import static org.monte.media.VideoFormatKeys.ENCODING_AVI_TECHSMITH_SCREEN_CAPTURE;
import static org.monte.media.VideoFormatKeys.QualityKey;

import java.awt.Dimension;
import java.awt.GraphicsConfiguration;
import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;

import org.monte.media.Format;
import org.monte.media.FormatKeys.MediaType;
import org.monte.media.math.Rational;
import org.monte.screenrecorder.ScreenRecorder;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.firefox.FirefoxBinary;
import org.openqa.selenium.firefox.FirefoxDriver;

public class CaptureScreen {
	private ScreenRecorder screenRecorder;

	public static void main(String[] args) throws Exception {

		String url = args[0]; // "http://www.aalto.fi/en/";
		String videoFileName = args[1];

		CaptureScreen videoReord = new CaptureScreen();

		WebDriver driver;
		FirefoxBinary binary = new FirefoxBinary(new File("/usr/bin/firefox"));

		driver = new FirefoxDriver(binary, null);
		driver.manage().deleteAllCookies();
		driver.manage().window().maximize();
		int height = driver.manage().window().getSize().getHeight();
		/**
		 * set the resolution of the xvfb server using /usr/bin/Xvfb :99 -ac
		 * -screen 0 1920x1200x24 & export DISPLAY=:99
		 * 
		 */
		int width = driver.manage().window().getSize().getWidth();

		long startTime = System.currentTimeMillis();
		// videoReord.startRecording(videoFileName, 1920, 1200);
		videoReord.startRecording(videoFileName, width, height);

		driver.get(url);
		/**
		 * Check if the screen is recorded for 30 seconds or more
		 */
		long finishTime = System.currentTimeMillis();
		long totalTime = finishTime -  startTime; // page load time as reported by the browser 
		try{
			PrintStream ps = new PrintStream(new FileOutputStream(videoFileName+".tmp"));
			ps.print(String.valueOf(totalTime));
			ps.close();
		}catch(IOException io){
			System.err.println("IO Error" + io.getMessage());
		}
		
		long sleepTime = 30000 - (totalTime);
		try {
			if (sleepTime > 0) {
				Thread.sleep(sleepTime);
			}
		} catch (InterruptedException ex) {
			Thread.currentThread().interrupt();
		}

		// System.out.println("Page title is: " + driver.getTitle());
		driver.quit();
		videoReord.stopRecording();
		// write resolution to a file
		if (!(new File("resolution")).exists()) {
			try {
				PrintStream out = new PrintStream(new FileOutputStream(
						"resolution"));
				out.print(String.valueOf(width) + "x" + String.valueOf(height));
				out.close();
			} catch (IOException io) {
				System.err.println("IO Error" + io.getMessage());
			}
		}

	}

	public void startRecording(String videoFileName, int width, int height)
			throws Exception {
		String workingDir = System.getProperty("user.dir") + "/";
		File file = new File(workingDir);

		// Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		// int width = screenSize.width;
		// int height = screenSize.height;

		Rectangle captureSize = new Rectangle(0, 0, width, height);

		GraphicsConfiguration gc = GraphicsEnvironment
				.getLocalGraphicsEnvironment().getDefaultScreenDevice()
				.getDefaultConfiguration();

		this.screenRecorder = new SpecializedScreenRecorder(
				gc,
				captureSize,
				new Format(MediaTypeKey, MediaType.FILE, MimeTypeKey, MIME_AVI),
				new Format(MediaTypeKey, MediaType.VIDEO, EncodingKey,
						ENCODING_AVI_TECHSMITH_SCREEN_CAPTURE,
						CompressorNameKey,
						ENCODING_AVI_TECHSMITH_SCREEN_CAPTURE, DepthKey, 24,
						FrameRateKey, Rational.valueOf(20), QualityKey, 1.0f,
						KeyFrameIntervalKey, 20 * 60), new Format(MediaTypeKey,
						MediaType.VIDEO, EncodingKey, "black", FrameRateKey,
						Rational.valueOf(10)), null, file, videoFileName);
		this.screenRecorder.start();

	}

	public void stopRecording() throws Exception {
		this.screenRecorder.stop();
	}

}
