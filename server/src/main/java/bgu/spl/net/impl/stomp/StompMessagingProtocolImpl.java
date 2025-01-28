package bgu.spl.net.impl.stomp;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<Frame> {

    private int connectionId;
    private ConnectionsImpl<Frame> connections;
    private boolean shouldTerminate = false;

    @Override
    public void start(int connectionId, Connections<Frame> connections) {
        this.connectionId = connectionId;
        // Cast to ConnectionsImpl<Frame>
        if (connections instanceof ConnectionsImpl) {
            this.connections = (ConnectionsImpl<Frame>) connections;
        } else {
            throw new IllegalArgumentException("Connections is not an instance of ConnectionsImpl");
        }
    }

    @Override
    public void process(Frame frame) {
        System.out.println("----Dafna that the frame you recive from the client:----- " + frame.toString());
        if (frame.getCommand().equalsIgnoreCase("CONNECT")) {
            handleConnectFrame(frame);
        } else if (frame.getCommand().equalsIgnoreCase("DISCONNECT")) {
            handleDisconnectFrame(frame);
        } else if (frame.getCommand().equalsIgnoreCase("SUBSCRIBE")) {
            handleSubscribeFrame(frame);
        } else if (frame.getCommand().equalsIgnoreCase("UNSUBSCRIBE")) {
            handleUnSubscribeFrame(frame);
        } else if (frame.getCommand().equalsIgnoreCase("SEND")) {
            handleSendFrame(frame);
        }

        else {
            System.out.println("Unknown command: " + frame.getCommand());
        }
    }

    private void handleUnSubscribeFrame(Frame frame) {
        String subscribtionId = frame.getHeader("id");
        if (connections.getHeplperTopics().get(connectionId) == null) {
            sendErrorAndClose(frame, "the client not subscribe to the channel.");
        } else if (connections.getHeplperTopics().get(connectionId).get(subscribtionId) == null) {
            sendErrorAndClose(frame, "the client not subscribe to the channel.");
        } else {
            connections.removeSubscription(connectionId, subscribtionId);
            sendReceiptFrameIfNeeded(frame);
        }

    }

    private void handleSubscribeFrame(Frame frame) {
        String channelName = frame.getHeader("destination");
        String subscribtionId = frame.getHeader("id");
        // Test 1: check if the it formed frame
        if (channelName == null || subscribtionId == null) {
            sendErrorAndClose(frame, "malformed frame received");
        } else {
            connections.addSubscription(channelName, subscribtionId, connectionId);
            sendReceiptFrameIfNeeded(frame);
        }
    }

    private void handleSendFrame(Frame frame) {
        // Tests:
        // Test 1: check if the frame is in the format
        if (frame.getHeader("destination") == null) {
            sendErrorAndClose(frame, "malformed frame received");
        } else {

            String channelName = frame.getHeader("destination");
            channelName = channelName.substring(1);
            if (connections.getTopics().get(channelName) == null) {
                sendErrorAndClose(frame, "No channel like this.");
            } else if (connections.getTopics().get(channelName).get(connectionId) == null) {
                sendErrorAndClose(frame, "The user is not subscribe to the chennel");
            } else { // Pass all the tests
                String frameString = frame.toString();

                // Required datafields
                String[] datafields = { "user", "destination", "event_name", "city", "date_time", "description",
                        "general_information", "active", "forces_arrival_at_scene" };
                List<String> frameHeadersList = Arrays.asList(datafields);

                // Initialize the ConcurrentHashMap
                ConcurrentHashMap<String, String> frameData = new ConcurrentHashMap<>();

                // Extract the content inside the curly braces { }
                String content = frameString.substring(frameString.indexOf("{") + 1, frameString.indexOf("}"));
                String[] pairs = content.split(","); // Split by commas for key-value pairs

                // Parse key-value pairs inside the curly braces
                for (String pair : pairs) {
                    String[] keyValue = pair.split("=", 2); // Split by '='
                    String key = keyValue[0].trim();
                    String value = keyValue.length > 1 ? keyValue[1].trim() : "";

                    if (key.equals("event name"))
                        key = "event_name"; // Normalize "event name" to "event_name"
                    if (key.equals("date time"))
                        key = "date_time"; // Normalize "date time" to "date_time"
                    if (key.equals("general information"))
                        key = "general_information"; // Normalize key
                    if (key.equals("forces arrival at scene"))
                        key = "forces_arrival_at_scene"; // Normalize key

                    // If the key is in the list of required fields, add it to frameData
                    if (frameHeadersList.contains(key)) {
                        frameData.put(key, value);
                    }
                }

                // Extract and handle additional fields outside the curly braces
                if (frame.toString().contains("user:")) {
                    String user = frameString.substring(frameString.indexOf("user:") + 5,
                            frameString.indexOf("\n", frameString.indexOf("user:"))).trim();
                    frameData.put("user", user);
                }

                if (frameString.toString().contains("description:")) {
                    String description = frameString.substring(frameString.indexOf("description:") + 12).trim();
                    frameData.put("description", description);
                }

                // Handle "active" separately, extracted from "general_information" if present
                if (frameData.containsKey("general_information")
                        && frameData.get("general_information").contains("active:")) {
                    String active = frameData.get("general_information").split(":")[1].trim();
                    frameData.put("active", active);
                }
            
            Frame messageFrame = new Frame("MESSAGE", null,null);
            frameData.forEach((key, value) -> messageFrame.addHeader(key, value));
                connections.send(channelName, messageFrame);
    sendReceiptFrameIfNeeded(frame);
            }
        }
    }
    // String[] datafields = {"user", "destination", "event_name", "city",
    // "date_time", "description",
    // "general_information", "active", "forces_arrival_at_scene"};
    // ConcurrentHashMap<String, String> frameData = new ConcurrentHashMap<>();
    // List<String> frameHeadersList = Arrays.asList(datafields);
    // String frameString = frame.toString();
    // // Create the frameData map
    // // Parse the given frame string
    // String content = frameString.substring(frameString.indexOf("{") + 1,
    // frameString.lastIndexOf("}")); // Extract content inside {}
    // String[] pairs = content.split(","); // Split by comma for key-value pairs

    // for (String pair : pairs) {
    // String[] keyValue = pair.split("=", 2); // Split each pair by '='
    // String key = keyValue[0].trim();
    // String value = keyValue.length > 1 ? keyValue[1].trim() : "";

    // // Add to frameData if the key is valid
    // if (frameHeadersList.contains(key)) {
    // frameData.put(key, value);
    // } else {
    // frameData.put("missing_key", key); // Handle unexpected keys
    // }
    // }

    // Frame messageFrame = new Frame("MESSAGE", null,null);
    // String subscriptionId=
    // connections.getTopics().get(channelName).get(connectionId);
    // messageFrame.addHeader("subscription", subscriptionId);
    // messageFrame.addHeader("user", frameData.get("user"));
    // String messageId = connections.getAndIncreceMessageId() +"";
    // messageFrame.addHeader("message-id", messageId);
    // messageFrame.addHeader("destination", frameData.get("destination"));
    // messageFrame.addHeader("event_name", frameData.get("event_name"));
    // messageFrame.addHeader("city", frameData.get("city"));
    // messageFrame.addHeader("date_time", frameData.get("date_time"));
    // messageFrame.addHeader("description", frameData.get("description"));
    // messageFrame.addHeader("general_information",
    // frameData.get("general_information"));
    // messageFrame.addHeader("active", frameData.get("active"));
    // messageFrame.addHeader("forces_arrival_at_scene",
    // frameData.get("forces_arrival_at_scene"));

    // System.out.println("---The one you send----" +messageFrame.toString());
    // connections.send(channelName, messageFrame);
    // sendReceiptFrameIfNeeded(frame);
    // }
    // String subscriptionId=
    // connections.getTopics().get(channelName).get(connectionId);
    // messageFrame.addHeader("subscription", subscriptionId);
    // String messageId = connections.getAndIncreceMessageId() +"";
    // messageFrame.addHeader("message-id",messageId);
    // messageFrame.addHeader("destination", channelName);
    // String theWholeHeadrs="";
    // for (Map.Entry<String, String> entry : frame.getHeaders().entrySet()) {
    // theWholeHeadrs=theWholeHeadrs+entry.getKey()+":"+entry.getValue()+"\n";
    // }
    // messageFrame.setBody(theWholeHeadrs+""+frame.getBody());
    // connections.send(channelName, messageFrame);
    // sendReceiptFrameIfNeeded(frame);
    // }
    // }

    private void handleDisconnectFrame(Frame frame) {
        sendReceiptFrameIfNeeded(frame);
        gracefulShoutDown();
    }

    private void handleConnectFrame(Frame frame) {
        String userName = frame.getHeader("login");
        String passward = frame.getHeader("passcode");
        boolean existsUser = connections.isUserExists(userName);
        boolean loginTestsSucced = true;
        if (existsUser) {
            if (connections.isClientExists(userName)) {
                loginTestsSucced = false;
                sendErrorAndClose(frame, "User is already logged in.");
            } else {
                System.out.println("Dafna you receach here:" + passward + " the pass in the map:"
                        + connections.getPassward(userName));
                if (!passward.equals(connections.getPassward(userName))) {
                    loginTestsSucced = false;
                    sendErrorAndClose(frame, "Wrong password.");
                } else {
                    connections.addClientToMapClientsNameAndId(userName, connectionId);
                }
            }
        } else {
            connections.addClientToMapAllUsers(userName, passward);
            connections.addClientToMapClientsNameAndId(userName, connectionId);
        }
        if (loginTestsSucced) {
            Frame connectedFrame = new Frame("CONNECTED", null, null);
            connectedFrame.addHeader("version", "1.2");
            System.out.println(connectedFrame.toString());
            if (!connections.send(connectionId, connectedFrame)) {
                System.out.println("ERROR: network disconnection");
            }
        }
        System.out.println("topics after login:" + connections.getTopics().toString());
        System.out.println("helperTopice after login:" + connections.getHeplperTopics().toString());

    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private void sendErrorAndClose(Frame frame, String errorDescription) {
        Frame errorFrame = new Frame("ERROR", null, null);
        errorFrame.addHeader("Error Description:", errorDescription);
        errorFrame.setBody("----------\n" + frame.toString() + "----------\n");
        if (!connections.send(connectionId, errorFrame)) {
            System.out.println("ERROR????????????");
        }
        sendReceiptFrameIfNeeded(errorFrame);
        gracefulShoutDown();
    }

    private void gracefulShoutDown() {
        shouldTerminate = true;
        connections.disconnect(connectionId);
    }

    private void sendReceiptFrameIfNeeded(Frame frame) {
        String receiptId = frame.getHeader("receipt");
        if (receiptId != null) {
            Frame receiptFrame = new Frame("RECEIPT", null, null);
            receiptFrame.addHeader("receipt-id", receiptId);
            if (!connections.send(connectionId, receiptFrame)) {
                System.out.println("ERROR: network disconnection");
            } else {
                System.out.println(receiptFrame.toString());
            }
        }
    }
}
