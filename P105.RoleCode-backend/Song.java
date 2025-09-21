import java.lang.StringBuilder;

public class Song implements Comparable<SongInterface>, SongInterface {
    private String title;
    private String artist;
    private String genre;
    private int year;
    private int bpm;
    private int energy;
    private int danceability;
    private int loudness;
    private int liveness;
  
    public Song(String input) {
	// note - helped by StackOverflow: https://stackoverflow.com/questions/1757065/java-splitting-a-comma-separated-string-but-ignoring-commas-in-quotes
	// build the list of strings to parse
	StringBuilder builder = new StringBuilder(input);
	boolean quoted = false;
	for (int i = 0; i < builder.length(); i++) {
		char chr = builder.charAt(i);
		// if we find a quote, toggle the quoted boolean
		if (chr == '\"') {
			quoted = !quoted;
		}
		// if we are NOT quoted, and we find a comma, change to a special separator for splitting
		if (!quoted && chr == ',') {
			builder.setCharAt(i, '@');
		}
	}
	String[] data = builder.toString().split("@");
	
	title = data[0];
	artist = data[1];
	genre = data[2];
	year = Integer.parseInt(data[3]);
	bpm = Integer.parseInt(data[4]);
	energy = Integer.parseInt(data[5]);
	danceability = Integer.parseInt(data[6]);
	loudness = Integer.parseInt(data[7]);
	liveness = Integer.parseInt(data[8]);
    }

    public String getTitle() {
    	return title;
    } // returns this song's title

    public String getArtist() {
	return artist;
    } // returns this song's artist

    public String getGenres() {
	return genre;
    } // returns string containing each of this song's genres

    public int getYear() {
	return year;
    }; // returns this song's year in the Billboard
    
    public int getBPM() {
	return bpm;
    } // returns this song's speed/tempo in beats per minute
    
    public int getEnergy() {
	return energy;
    } // returns this song's energy rating 
    
    public int getDanceability() {
	return danceability;
    } // returns this song's danceability rating
    
    public int getLoudness() {
	return loudness;
    } // returns this song's loudness in dB
    
    public int getLiveness() {
	return liveness;
    } // returns this song's liveness rating
    
    // compares the danceability of songs
    public int compareTo(SongInterface song) {
	return this.getDanceability() - song.getDanceability();
    }
}

