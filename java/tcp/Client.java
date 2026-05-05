import java.io.*;
import java.net.*;

public class Client {
    public static void main(String[] args) throws IOException {
        // Connect to server at localhost on port 8080
        Socket socket = new Socket("172.21.3.58", 8081);
        System.out.println("Connected to server.");

        // Set up streams
        BufferedReader in = new BufferedReader(
            new InputStreamReader(socket.getInputStream()));
        PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
        BufferedReader keyboard = new BufferedReader(
            new InputStreamReader(System.in));

        // Send messages typed by the user
        String userInput;
        System.out.print("Enter message: ");
        while ((userInput = keyboard.readLine()) != null) {
            out.println(userInput);                         // send to server
            System.out.println(in.readLine());              // print server reply

            if (userInput.equalsIgnoreCase("bye")) break;
            System.out.print("Enter message: ");
        }

        // Close everything
        in.close();
        out.close();
        keyboard.close();
        socket.close();
    }
}