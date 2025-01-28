package bgu.spl.net.impl.stomp;
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
           }
           else if (frame.getCommand().equalsIgnoreCase("DISCONNECT")) {
            handleDisconnectFrame(frame);
           }
           else if (frame.getCommand().equalsIgnoreCase("SUBSCRIBE")) {
            handleSubscribeFrame(frame);
           }
           else if (frame.getCommand().equalsIgnoreCase("UNSUBSCRIBE")){
            handleUnSubscribeFrame(frame);
           }
           else if (frame.getCommand().equalsIgnoreCase("SEND")){
            handleSendFrame(frame);
           }
           
         else {
            System.out.println("Unknown command: " + frame.getCommand());
        }
    }

    private void handleUnSubscribeFrame(Frame frame){
        String subscribtionId = frame.getHeader("id");
        if (connections.getHeplperTopics().get(connectionId)==null){
            sendErrorAndClose(frame, "the client not subscribe to the channel.");
        }
        else if (connections.getHeplperTopics().get(connectionId).get(subscribtionId)==null) {
            sendErrorAndClose(frame, "the client not subscribe to the channel.");
        }
        else {
            connections.removeSubscription(connectionId,subscribtionId);
            sendReceiptFrameIfNeeded(frame);
        }
      
    }
        

    private void handleSubscribeFrame(Frame frame){
        String channelName = frame.getHeader("destination");
        String subscribtionId = frame.getHeader("id");
        //Test 1: check if the it formed frame 
        if (channelName==null ||subscribtionId==null){
            sendErrorAndClose(frame, "malformed frame received");
        }
        else {
            connections.addSubscription(channelName, subscribtionId, connectionId);
            sendReceiptFrameIfNeeded(frame);
        }
    }

    private void handleSendFrame (Frame frame){
        //Tests:
        //Test 1: check if the frame is in the format
        if (frame.getHeader("destination") ==null){
            sendErrorAndClose(frame, "malformed frame received");
        }
        //Test 2: check if the sender is subscribed to the channnel
        else {
            String channelName = frame.getHeader("destination");
            // String channelNameOrigin = frame.getHeader("destination");
            // if (channelName.charAt(0) == '/') {
            //     channelName = channelName.substring(1); // Remove the first character
            // }
            // System.out.println("DAFNA2:" + channelName);
            if (connections.getTopics().get(channelName)==null){
                sendErrorAndClose(frame, "No channel like this.");
            }
            else if (connections.getTopics().get(channelName).get(connectionId)==null){
                    sendErrorAndClose(frame, "The user is not subscribe to the chennel");
                }
            else {   //Pass all the tests
                Frame messageFrame = new Frame("MESSAGE", null,null);
                String subscriptionId= connections.getTopics().get(channelName).get(connectionId);
                messageFrame.addHeader("subscription", subscriptionId);
                String messageId = connections.getAndIncreceMessageId() +"";
                messageFrame.addHeader("message-id",messageId);
                messageFrame.addHeader("destination", channelName);
                messageFrame.setBody(frame.getBody());
                connections.send(channelName, messageFrame);
                sendReceiptFrameIfNeeded(frame); 
                }       
            }    
        }


    private void handleDisconnectFrame (Frame frame){
        sendReceiptFrameIfNeeded(frame);
        gracefulShoutDown();
    }
    
    private void handleConnectFrame (Frame frame){
        String userName = frame.getHeader("login"); 
        String passward = frame.getHeader("passcode");
        boolean existsUser = connections.isUserExists(userName);
        boolean loginTestsSucced = true; 
        if (existsUser) {
            if (connections.isClientExists(userName)){
                loginTestsSucced=false; 
                sendErrorAndClose(frame, "User is already logged in.");
            }
            else { 
                System.out.println("Dafna you receach here:" + passward + " the pass in the map:" +connections.getPassward(userName));
                if (!passward.equals(connections.getPassward(userName))){
                    loginTestsSucced=false; 
                   sendErrorAndClose(frame, "Wrong password.");
                }
                else {
                    connections.addClientToMapClientsNameAndId(userName, connectionId);
                }
            }
        }
        else {
            connections.addClientToMapAllUsers(userName, passward);
            connections.addClientToMapClientsNameAndId(userName, connectionId);
        }
        if (loginTestsSucced){
        Frame connectedFrame = new Frame("CONNECTED", null,null);
                    connectedFrame.addHeader("version", "1.2");
                    System.out.println(connectedFrame.toString());
                    if (!connections.send(connectionId, connectedFrame)){
                        System.out.println("ERROR: network disconnection");
                       }
                    }
    System.out.println("topics after login:" +connections.getTopics().toString());
    System.out.println("helperTopice after login:" +connections.getHeplperTopics().toString());

    }
    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private void sendErrorAndClose (Frame frame, String errorDescription){
        Frame errorFrame = new Frame("ERROR", null,null);
        errorFrame.addHeader("Error Description:", errorDescription);
        errorFrame.setBody("----------\n" + frame.toString() + "----------\n");
        if (!connections.send(connectionId, errorFrame)){
            System.out.println("ERROR????????????");
           }
        sendReceiptFrameIfNeeded(errorFrame);
        gracefulShoutDown();
    }

    private void gracefulShoutDown(){
        shouldTerminate=true;
        connections.disconnect(connectionId);
    }

    private void sendReceiptFrameIfNeeded(Frame frame){
        String receiptId= frame.getHeader("receipt");
        if (receiptId!=null){
            Frame receiptFrame = new Frame("RECEIPT", null, null);
            receiptFrame.addHeader("receipt-id", receiptId);
            if (!connections.send(connectionId, receiptFrame)){
                System.out.println("ERROR: network disconnection");
               }
            else{
                System.out.println(receiptFrame.toString());
            }
        }
    }
}
