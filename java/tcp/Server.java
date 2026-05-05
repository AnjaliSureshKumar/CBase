import java.io.*;
import java.net.*;
import java.util.Scanner;

public class Server {
    public static void main(String[] args) throws IOException {
        // Create server socket on port 8080
        ServerSocket serverSocket = new ServerSocket(8080);
        System.out.println("Server started. Waiting for client...");

        // Accept incoming client connection (blocks until client connects)
        Socket clientSocket = serverSocket.accept();
        System.out.println("Client connected: " + clientSocket.getInetAddress());

        // Set up input/output streams
        BufferedReader in = new BufferedReader(
            new InputStreamReader(clientSocket.getInputStream()));
        PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true);

        // Read message from client and echo it back
        String message;
        Scanner scanner = new Scanner(System.in);
        while ((message = in.readLine()) != null) {
            System.out.println("Client says: " + message);
            System.out.print("Server: ");
            String response = scanner.nextLine();
            out.println("Server : " + response);

            if (message.equalsIgnoreCase("bye")) {
                System.out.println("Client disconnected.");
                break;
            }
        }

        // Close everything
        in.close();
        out.close();
        clientSocket.close();
        serverSocket.close();
    }
}