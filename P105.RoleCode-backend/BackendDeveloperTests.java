import org.junit.jupiter.api.*;
import java.io.IOException;
import java.io.File;
import java.util.*;

/**
 * This class provides tester methods that test the functionality
 * of the BackendPlaceholder class.
 *
 * @author Yayan Deng
 */
 public class BackendDeveloperTests {
	
	/*
	 * This test makes sure that readData() throws an IOException, and not any other exception, when
	 * given a bad file path.
	 */
	 @Test
	public void readDataTestBad() {
		Backend backend = new Backend();

		// test IOException
		boolean failedBad = true; // initiate to true, can set false later
		try {
			backend.readData("NOT A FILE");
			// we expect an exception to be thrown here
		} catch (IOException e) {
			// good, set failedBad to false
			failedBad = false;
		} catch (Exception e) {
			// caught any other exception, failedBad is true
			failedBad = true;
		}

		Assertions.assertTrue(!failedBad, "readDataTestBad() failed");
	}
	
	/*
	 * This test makes sure that readData() does NOT throw an exception
	 * when given a good file path.
	 */
	@Test
	public void readDataTestGood() {
		Backend backend = new Backend();

		// test good file
		boolean failedGood = false; // initiate to false, can set to true later
		try {
			backend.readData("." + File.separator + "songs.csv");
			// we do NOT expect this to throw an exception
		} catch (Exception e) {
			// bad, we don't want ANY exception to be thrown here
			failedGood = true;
		}

		Assertions.assertTrue(!failedGood, "readDataTestGood() failed");
	}
	
	/*
	 * This test tests the functionality of the getRange() method of the
	 * BackendPlaceholder class.
	 */
	@Test
	public void getRangeTest() {
		Backend backend = new Backend();
		
		// initialize the expected ArrayList of song names
		ArrayList<String> expected = new ArrayList<String>(Arrays.asList(
			"Bound To You - Burlesque Original Motion Picture Soundtrack",
			"You Lost Me",
			"1+1",
			"Turning Page",
			"Atlas - From �The Hunger Games: Catching Fire� Soundtrack",
			"Chandelier",
			"\"Love Me Like You Do - From \"\"Fifty Shades Of Grey\"\"\"",
			"Animals",
			"St Jude",
			"Run Run Run",
			"Million Years Ago",
			"Free Me",
			"Dusk Till Dawn - Radio Edit"));

		// try to read data in case IOException is thrown
		try {
			backend.readData("songs.csv");
			// if no exception is thrown, then check if expected is the same as actual
			
			// case1: getRange(1000,1000)
			ArrayList<String> actual = new ArrayList<String>(backend.getRange(1000, 1000));
			// should have no songs in this range
			if (!actual.isEmpty()) {
				Assertions.fail("getRange(1000,1000) unexpected output");
			}

			// case2: getRange(0,30), should be equal to expected
			actual = new ArrayList<String>(backend.getRange(0, 30));
			Assertions.assertTrue(expected.equals(actual), "getRangeTest() failed");
		} catch (IOException e) { // fail the test if an IOException was thrown
			Assertions.fail("Threw an IOException!");
		} catch (Exception e) { // fail if ANY exception is thrown
                        Assertions.fail("Threw an exception: " + e.getMessage());
                }
	}
	
	/*
         * This test tests the functionality of the filterEnergeticSongs() method of the
         * BackendPlaceholder class.
         */
	@Test
	public void filterEnergeticSongsTest() {
		Backend backend = new Backend();
		
		// initialize the expected ArrayList of song names
		ArrayList<String> expected = new ArrayList<String>(Arrays.asList(
                        "Chandelier",
                        "\"Love Me Like You Do - From \"\"Fifty Shades Of Grey\"\"\"",
                        "Animals"));
		
		// try to read data in case IOException is thrown
		try {
			backend.readData("songs.csv");
			// case 1: getRange was NOT previously called - should return empty list
			ArrayList<String> actual = new ArrayList<String>(backend.filterEnergeticSongs(100));
			if (!actual.isEmpty()) {
				Assertions.fail("List is NOT empty even though getRange() was never called!");
			}

			// case 2: getRange() called, but minEnergy unreasonable at 1000; actual should be empty list
			backend.getRange(0,30);
			actual = new ArrayList<String>(backend.filterEnergeticSongs(1000));
			if (!actual.isEmpty()) {
				Assertions.fail("filterEnergeticSongs(1000) unexpected output");
			}

			// case 3: getRange() called, minEnergy is reasonable; check if expected is the same as actual
			actual = new ArrayList<String>(backend.filterEnergeticSongs(50));
			Assertions.assertTrue(expected.equals(actual), "filterEnergeticSongsTest() failed");
		} catch (IOException e) { // fail the test if an IOException was thrown
                        Assertions.fail("Threw an IOException!");
                } catch (Exception e) { // fail if ANY exception is thrown
                        Assertions.fail("Threw an exception: " + e.getMessage());
                }
	}
	
	/*
         * This test tests the functionality of the fiveFastest() method of the
         * BackendPlaceholder class.
         */
	@Test
	public void fiveFastestTest() {
		Backend backend = new Backend();

		// initialize the expected ArrayList of song names
		ArrayList<String> expected = new ArrayList<String>(Arrays.asList(
			"164: Bound To You - Burlesque Original Motion Picture Soundtrack",
			"174: Chandelier",
			"180: Dusk Till Dawn - Radio Edit",
			"190: \"Love Me Like You Do - From \"\"Fifty Shades Of Grey\"\"\"",
			"190: Animals"));

		// run fiveFastest() WITHOUT calling getRange() - we expect an IllegalStateException
		// try to read data in case IOException is thrown
		try {
			backend.readData("songs.csv");
			backend.fiveFastest();
			// if no exception is thrown, then fail the test
			Assertions.fail("No IllegalstateException thrown even though getRange was never called!");
		} catch (IOException e) { // fail the test if an IOException was thrown
                        Assertions.fail("Threw an IOException!");
                } catch (IllegalStateException e) {
			// good, we wanted to catch this exception since getRange() was never previously called
		}

		// now, run fiveFastest() AFTER calling getRange()
		// if no exception is thrown, then check if expected is the same as actual
		try {
			backend.getRange(0, 30);
			ArrayList<String> actual = new ArrayList<String>(backend.fiveFastest());
			Assertions.assertTrue(expected.equals(actual), "fiveFastestTest() failed");
		} catch (Exception e) { // fail if ANY exception is thrown
			Assertions.fail("Threw an exception: " + e.getMessage());
		}
	}
}
