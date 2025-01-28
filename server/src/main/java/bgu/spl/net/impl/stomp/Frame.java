package bgu.spl.net.impl.stomp;
import java.util.HashMap;
import java.util.Map;

public class Frame {
    private String command;
    private Map<String, String> headers;
    private String body;

    // Constructor
    public Frame(String command, Map<String, String> headers, String body) {
        this.command = command;
        this.headers = headers != null ? headers : new HashMap<>();
        this.body = body;
    }

    // Getters
    public String getHeader(String key) {
        return headers.get(key);
    }
    public String getCommand() {
        return command;
    }

    public Map<String, String> getHeaders() {
        return headers;
    }

    public String getBody() {
        return body;
    }

    // Serialization: Converts the frame to a STOMP frame string
    public String serialize() {
        StringBuilder frame = new StringBuilder();
        frame.append(command).append("\n");
        
        for (Map.Entry<String, String> header : headers.entrySet()) {
            frame.append(header.getKey()).append(":").append(header.getValue()).append("\n");
        }
        
        frame.append("\n"); // Blank line before the body
        if (body != null) {
            frame.append(body);
        }
        frame.append("\0"); // Null character indicating the end of the frame
        return frame.toString();
    }

    // Deserialization: Creates a StompFrame from a STOMP frame string
    public static Frame deserialize(String frameData) {
        String[] lines = frameData.split("\n", -1); // Include trailing empty strings
        if (lines.length == 0) {
            throw new IllegalArgumentException("Invalid STOMP frame format");
        }

        String command = lines[0];
        Map<String, String> headers = new HashMap<>();
        StringBuilder bodyBuilder = new StringBuilder();
        boolean bodyStarted = false;

        for (int i = 1; i < lines.length; i++) {
            String line = lines[i];

            if (bodyStarted) {
                bodyBuilder.append(line);
                if (i < lines.length - 1) {
                    bodyBuilder.append("\n");
                }
            } else if (line.isEmpty()) {
                bodyStarted = true;
            } else {
                int separatorIndex = line.indexOf(':');
                if (separatorIndex == -1) {
                    throw new IllegalArgumentException("Invalid header format: " + line);
                }
                String key = line.substring(0, separatorIndex);
                String value = line.substring(separatorIndex + 1);
                headers.put(key, value);
            }
        }

        // Remove the trailing null character from the body
        String body = bodyBuilder.toString();
        if (!body.isEmpty() && body.charAt(body.length() - 1) == '\0') {
            body = body.substring(0, body.length() - 1);
        }

        return new Frame(command, headers, body);
    }

    // For debugging purposes
    @Override
    public String toString() {
        return "---------\n" +
                 command + "\n" +
                 headers + "\n" +
                 body + "\n -------- \n"
                ;
    }

    public void addHeader(String string, String string2) {
        headers.put(string, string2);
    }
    public void setBody(String body) {
        this.body = body;
    }
}

 
