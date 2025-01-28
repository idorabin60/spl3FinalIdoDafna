package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.StompMessagingProtocol;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.Socket;

public class BlockingConnectionHandler<T> implements Runnable, ConnectionHandler<T> {

    private final StompMessagingProtocol<T> protocol;
    private final MessageEncoderDecoder<T> encdec;
    private final Socket sock;
    private BufferedInputStream in;
    private BufferedOutputStream out;
    private volatile boolean connected = true;

    public BlockingConnectionHandler(Socket sock, MessageEncoderDecoder<T> reader, StompMessagingProtocol<T> protocol, int connectionId, Connections<T> connections) {
        this.sock = sock;
        this.encdec = reader;
        this.protocol = protocol; 
        this.protocol.start(connectionId, connections); // Initialize the protocol with connection-specific data
    }

    @Override
    public void run() {
        try (Socket sock = this.sock) { //just for automatic closing
            int read;

            in = new BufferedInputStream(sock.getInputStream());
            out = new BufferedOutputStream(sock.getOutputStream());

            while (!protocol.shouldTerminate() && connected && (read = in.read()) >= 0) {
                T nextMessage = encdec.decodeNextByte((byte) read);
                if (nextMessage != null) {
                    protocol.process(nextMessage);
                }
            }

        } catch (IOException ex) {
            ex.printStackTrace();
        }

    }

    @Override
public void close() throws IOException {
    connected = false;
    sock.close();

    // // Process all remaining messages in the input stream
    // try {
    //     int read;
    //     while (in != null && (read = in.read()) >= 0) {
    //         T nextMessage = encdec.decodeNextByte((byte) read);
    //         if (nextMessage != null) {
    //             protocol.process(nextMessage); // Ensure remaining messages are processed
    //         }
    //     }
    // } catch (IOException e) {
    //     System.err.println("Error processing remaining messages: " + e.getMessage());
    // }

    // // Close the socket to release resources
    // try {
    //     if (sock != null) {
    //         sock.close();
    //         System.out.println("Socket closed successfully.");
    //     }
    // } catch (IOException e) {
    //     System.err.println("Error closing socket: " + e.getMessage());
    // }
}


    // @Override
    // public void close() throws IOException {
    //     System.out.println("i reach the clossssssse");
    //     connected = false;
    //     sock.close();
    // }

    @Override
    public void send(T msg) {
        if (connected) {
            try {
                // Encode the message into bytes using the encoder/decoder
                byte[] encodedMessage = encdec.encode(msg);
    
                // Write the encoded message to the output stream
                out.write(encodedMessage);
    
                // Flush the output stream to ensure the data is sent immediately
                out.flush();
            } catch (IOException ex) {
                ex.printStackTrace();
                connected = false; // Mark the connection as closed on failure
            }
        }
    }
    
}
