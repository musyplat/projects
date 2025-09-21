import java.util.Scanner;

/**
 * FrontendPlaceholder - CS400 Project 1: iSongify
 *
 * This class doesn't really work.  The methods are hardcoded to output values
 * as placeholders throughout development.  It demonstrates the architecture
 * of the Frontend class that will be implemented in a later week.
 */
public class FrontendPlaceholder implements FrontendInterface {

    private String min = "min";
    private String max = "max";
    private String energy = "none";
    
    public FrontendPlaceholder(Scanner in, BackendInterface backend) {}

    /**
     * Repeated gives the user an opportunity to issue new commands until
     * they select Q to quit.
     */
    public void runCommandLoop() {
	displayMainMenu();
	System.out.println("R"); // user entered command
	readFile();

	displayMainMenu();
	System.out.println("G"); // user entered command
	getValues();

	displayMainMenu();
	System.out.println("F"); // user entered command
	setFilter();

	displayMainMenu();
	System.out.println("D"); // user entered command
	topFive();

    	displayMainMenu();
	System.out.println("Q"); // user entered command
    }

    /**
     * Displays the menu of command options to the user.
     */
    public void displayMainMenu() {
		
	String menu = """
	    
	    ~~~ Command Menu ~~~
	        [R]ead Data
	        [G]et Songs by Danceability [min - max]
	        [F]ilter New Songs (by Min Energy: none)
	        [D]isplay Five Fastest
	        [Q]uit
	    Choose command:""";
	menu=menu.replace("min",min).replace("max",max).replace("none",energy);
	System.out.print(menu + " ");
    }

    /**
     * Provides text-based user interface and error handling for the 
     * [R]ead Data command.
     */
    public void readFile() {
	System.out.print("Enter path to csv file to load: ");
	
	System.out.println("mySongData.csv"); // user entered command

	System.out.println("Done reading file.");
    }
    
    /**
     * Provides text-based user interface and error handling for the 
     * [G]et Songs by Danceability command.
     */
    public void getValues() {
	System.out.print("Enter range of values (MIN - MAX): ");

	System.out.println("80 - 90"); // user entered command
	min="80";
	max="90";
	
	System.out.println("""
			   5 songs found between 80 - 90:
			       Baby
			       Dynamite
			       Secrets
			       Empire State of Mind (Part II) Broken Down
			       Only Girl (In The World)""");
    }

    /**
     * Provides text-based user interface and error handling for the 
     * [F]ilter Energetic Songs (by Min Energy) command.
     */
    public void setFilter() {
	System.out.print("Enter minimum energy: ");
	System.out.println(85);
	energy="85";
	
	System.out.println("""
			   2 songs found between 80 - 90 with energy >= 85:
			       Baby
			       Only Girl (In The World)""");
    }

    /**
     * Provides text-based user interface and error handling for the 
     * [D]isplay Five Fastest command.
     */
    public void topFive() {
	System.out.println("""
			   Top Five songs found between 80 - 90 with energy >= 85:
			       89: Baby
			       93: Only Girl (In The World)""");
    }

}
