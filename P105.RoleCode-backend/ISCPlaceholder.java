import java.util.Iterator;
import java.util.ArrayList;

public class ISCPlaceholder<T extends Comparable<T>> extends ArrayList<T>
    implements IterableSortedCollection<T> {
    // TODO: currently implemented a temporary data structure (arrayList) for use in P105
    //       need to convert this to a red-black tree structure in subsequent weeks

    private T value;
    private ArrayList<T> list = new ArrayList<>(); // TODO: remove later
    
    public boolean insert(T data)
	throws NullPointerException, IllegalArgumentException {
	list.add(data);
	// value = data;
	return true; // placeholder assuming insert is always successful
    }

    public boolean contains(Comparable<T> data) {
	return list.contains(data);
	// return true;
    }

    public boolean isEmpty() {
	return list.isEmpty();
	// return false;
    }
    
    public int size() {
	return list.size();
	// return 3;
    }

    public void clear() {
	list = new ArrayList<T>();    
    }

    public void setIterationStartPoint(Comparable<T> startPoint) {	
    }

    public Iterator<T> iterator() {	
	return list.iterator();
	// return java.util.Arrays.asList(value, value, value).iterator();
    }
}
