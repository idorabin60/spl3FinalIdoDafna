package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        System.err.println("Hello Dafna");
        if (args.length < 2) {
            System.err.println("Usage: java -jar StompServer.jar <port> <reactor/tpc>");
            return;
        }

        int port = Integer.parseInt(args[0]);
        String serverType = args[1];

        if (serverType.equalsIgnoreCase("tpc")) {
            // Thread-Per-Client Server
            Server.threadPerClient(
                port,
                StompMessagingProtocolImpl::new, // Protocol factory
                MessageEncoderDecoderImpl::new // Encoder/Decoder factory
            ).serve();
        } else if (serverType.equalsIgnoreCase("reactor")) {
            // Reactor Server
            Server.reactor(
                Runtime.getRuntime().availableProcessors(), // Number of threads
                port,
                StompMessagingProtocolImpl::new, // Protocol factory
                MessageEncoderDecoderImpl::new // Encoder/Decoder factory
            ).serve();
        } else {
            System.err.println("Invalid server type. Use 'reactor' or 'tpc'.");
        }
    }
}
