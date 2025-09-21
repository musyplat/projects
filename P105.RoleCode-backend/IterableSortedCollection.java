/**
 * This ADT supports iteration through a sorted collection.
 * 
 * When a non-null start point is set, the first value returned by this 
 * iterator will be the smallest value in the collection that is smaller than 
 * or equal to the specified start point.  When no (or a null) start point is 
 * set, the iterator will step through all values in the collection.
 */
public interface IterableSortedCollection <T extends Comparable<T>>
    extends SortedCollectionInterface<T>, Iterable<T> {

    public void setIterationStartPoint(Comparable<T> startPoint);

}
