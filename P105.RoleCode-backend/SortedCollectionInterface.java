public interface SortedCollectionInterface<T extends Comparable<T>> {

    public boolean insert(T data)
	throws NullPointerException, IllegalArgumentException;

    public boolean contains(Comparable<T> data);

    public boolean isEmpty();
    
    public int size();

    public void clear();

}
