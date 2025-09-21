import java.util.*;
import java.io.*;

/**
 * Backend - CS400 Project 1: iSongify
 *
 * Backend for the iSongify project...
 */
public class Backend implements BackendInterface {
    // instance fields
    // tree to store data
    private IterableSortedCollection<SongInterface> tree = new ISCPlaceholder<SongInterface>();
    // store list of songs output from getRange()
    private ArrayList<String> rangedSongs = new ArrayList<String>();
    // store the input danceability of getRange()
    private int minRange = Integer.MIN_VALUE;
    private int maxRange = Integer.MAX_VALUE;
    // store the minEnergy input from filterEnergeticSongs()
    private int minEnergyThreshold = Integer.MIN_VALUE; // initialized w/ no "filter/min" by default

    // constructors
    public Backend() {
        // change nothing
    }

    public Backend(IterableSortedCollection<SongInterface> tree) {
    	this.tree = tree;
    }

    /**
     * Loads data from the .csv file referenced by filename.
     * @param filename is the name of the csv file to load data from
     * @throws IOException when there is trouble finding/reading file
     */
    public void readData(String filename) throws IOException {
	try {
		File data = new File(filename);
		Scanner sc = new Scanner(data, "utf-8");
		sc.nextLine(); // skip first line of file
      		while (sc.hasNext()) {
               		String row = sc.nextLine();
                	Song song = new Song(row);
                	tree.insert(song);
        	}
        	sc.close();
	} catch (FileNotFoundException e) {
		throw new IOException("File not found!");
	} catch (Exception e) {
		throw new IOException("Ran into trouble reading file!");
	}
    }

// TODO: SORTING BY DANCEABILITY WILL OCCUR AUTOMATICALLY WHEN IterableSortedCollection IS IMPLEMENTED
//      The structure will automatically be sorted (red-black tree) by danceability.
//      For now, we assume the songs are already inserted in order.
    
    /**
     * Retrieves a list of song titles for songs that have a Danceability
     * within the specified range (sorted by Danceability in ascending order).
     * If a minEnergy filter has been set using filterEnergeticSongs(), then 
     * only songs with an energy rating greater than or equal to this minEnergy
     * should beincluded in the list that is returned by this method.
     *
     * Note that either this danceability range, or the resulting unfiltered 
     * list of songs should be saved for later use by the other methods 
     * defined in this class.
     *
     * @param low is the minimum Danceability of songs in the returned list
     * @param hight is the maximum Danceability of songs in the returned list
     * @return List of titles for all songs in specified range 
     */
    public List<String> getRange(int low, int high) {
	ArrayList<String> songList = new ArrayList<String>();

	// filter the songs by danceability while also filtering by minEnergyThreshold
	for (SongInterface song : tree) {
		if (song.getDanceability() >= low && song.getDanceability() <= high && song.getEnergy() >= minEnergyThreshold) {
			songList.add(song.getTitle());
		}
	}
	
	// store the songList for later use in other methods, then return it
	// also store the input values, low and high
	minRange = low;
	maxRange = high;
	rangedSongs = songList;
    	return songList;
    }

    /**
     * Filters the list of songs returned by future calls of getRange() and 
     * fiveFastest() to only include energetic songs.  If getRange() was 
     * previously called, then this method will return a list of song titles
     * (sorted in ascending order by Danceability) that only includes songs on
     * with the specified minEnergy or higher.  If getRange() was not 
     * previously called, then this method should return an empty list.
     *
     * Note that this minEnergy threshold should be saved for later use by the 
     * other methods defined in this class.
     *
     * @param minEnergy is the minimum energy of songs that are returned
     * @return List of song titles, empty if getRange was not previously called
     */
    public List<String> filterEnergeticSongs(int minEnergy) {
    	// edge case: if getRange() was NOT previously called, then the rangedSongs list will be empty
	// return an empty list
	if (rangedSongs.isEmpty()) {
		return new ArrayList<String>();
	}
	
	ArrayList<String> energeticSongs = new ArrayList<String>();
	// otherwise, return songs from rangedSongs that have an energy level at least minEnergy
	for (SongInterface song : tree) {
		if (rangedSongs.contains(song.getTitle()) && song.getEnergy() >= minEnergy) {
			energeticSongs.add(song.getTitle());
		}
	}
	
	// store the minEnergy value, then return energeticSongs
	minEnergyThreshold = minEnergy;
	return energeticSongs;
    }

    /**
     * This method makes use of the attribute range specified by the most
     * recent call to getRange().  If a minEnergy threshold has been set by
     * filterEnergeticSongs() then that will also be utilized by this method.
     * Of those songs that match these criteria, the five fastest will be 
     * returned by this method as a List of Strings in increasing order of 
     * danceability.  Each string contains the speed in BPM followed by a 
     * colon, a space, and then the song's title.
     * If fewer than five such songs exist, display all of them.
     *
     * @return List of five fastest song titles and their respective speeds
     * @throws IllegalStateException when getRange() was not previously called.
     */
    public List<String> fiveFastest() {
	// edge case: getRange() was not previously called so throw IllegalStateException
	if (rangedSongs.isEmpty()) {
		throw new IllegalStateException("The getRange() method was never called!");
	}
	
	// filter songs based on attributes set by most recent calls to getRange() and filterEnergeticSongs();
	ArrayList<Song> filteredSongs = new ArrayList<Song>();
	for (SongInterface song : tree) {
		if (song.getDanceability() >= minRange && song.getDanceability() <= maxRange && song.getEnergy() >= minEnergyThreshold) {
			filteredSongs.add((Song) song);
		}
	}

	// sorting by BPM in ascending order instead of danceability, will be improved upon the implementation of the actual tree in coming weeks
        filteredSongs.sort(Comparator.comparingInt(SongInterface::getBPM));
	
	// now find the fastest 5 songs based on BPM
	ArrayList<String> fastestSongs = new ArrayList<String>();
	int initialIdx = Math.max(0, filteredSongs.size() - 5);
    	for (int i = initialIdx; i < Math.min(initialIdx + 5, filteredSongs.size()); i++) {
            Song song = filteredSongs.get(i);
            fastestSongs.add(song.getBPM() + ": " + song.getTitle());
        }

        return fastestSongs;
    }    
}
