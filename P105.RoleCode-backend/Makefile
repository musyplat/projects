runBDTests: ../junit5.jar BackendDeveloperTests.java
	javac Song.java
	javac Backend.java
	javac -cp .:../junit5.jar BackendDeveloperTests.java 
	java -jar ../junit5.jar -cp . -c BackendDeveloperTests

runBackendTests: runBDTests

clean:
	rm *.class
