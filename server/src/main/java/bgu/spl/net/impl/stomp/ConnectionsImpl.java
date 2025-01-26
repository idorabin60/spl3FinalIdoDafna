package bgu.spl.net.impl.stomp;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import javax.print.DocFlavor.STRING;

import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;

public class ConnectionsImpl<T> implements Connections<T> {

    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> connectionHandlers; // Maps connectionId to handlers
    private final ConcurrentHashMap<String, Integer> MapClientsNameAndId;
    private final ConcurrentHashMap<Integer, String> helperMapClientsNameAndId; 
    private final ConcurrentHashMap<String, ConcurrentHashMap<Integer,String>> topics; 
    //<String channelName, Map<int connectionId, String subscriptionId>> 
    private final ConcurrentHashMap<String, String> mapAllUsers;
    private final ConcurrentHashMap<Integer, ConcurrentHashMap<String,String>> helperTopics; 
    //<int connectionID, Map<String subscriptionId, String channelName>
    private AtomicInteger counterMassageId; 

    public ConnectionsImpl() {
        this.connectionHandlers = new ConcurrentHashMap<>();
        this.MapClientsNameAndId=new ConcurrentHashMap<>();
        this.topics=new ConcurrentHashMap<>();
        this.mapAllUsers = new ConcurrentHashMap<>();
        this.helperTopics = new ConcurrentHashMap<>();
        this.helperMapClientsNameAndId=new ConcurrentHashMap<>();
        this.counterMassageId = new AtomicInteger(0); 
    }

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = connectionHandlers.get(connectionId);
        if (handler != null) {
            handler.send(msg); // Use the send method of the ConnectionHandler
            return true;
        }
        return false; // Return false if the connectionId is not found
    }

    @Override
    public void send(String channel, T msg) {
        ConcurrentHashMap<Integer, String> subscribers = topics.get(channel);
        System.out.println(subscribers.size());
        if (msg instanceof Frame) {
            System.out.println("Dafna this is the message you broadcast:" + msg.toString());
        }
        ConnectionHandler<T> client; 
        if (subscribers != null) {
            for (Integer connectionId : subscribers.keySet()) {
                client = connectionHandlers.get(connectionId);
                client.send(msg); 
            }
        }
    }

    @Override
    public void disconnect(int connectionId) {
        //Removing from server data: 
       
        ConcurrentHashMap<String, String> allSubscriptions = helperTopics.get(connectionId);
        if (allSubscriptions != null) {
            // Iterate over all topics the connection is subscribed to
            for (String subscribtionId : allSubscriptions.keySet()) {
                String channelName = allSubscriptions.get(subscribtionId); 
                topics.get(channelName).remove(connectionId);
                }
         helperTopics.remove(connectionId).clear();;
        }
        System.out.println("--helperTopic map after dissconnet--\n" + topics.toString());
        System.out.println("--topices map after dissconnest--\n" + helperTopics.toString());

      String userName = helperMapClientsNameAndId.get(connectionId);
      if (userName!=null){
      helperMapClientsNameAndId.remove(connectionId);
      MapClientsNameAndId.remove(userName);
      }
      System.out.println("--helperMapClientsNameAndId map after dissconnest--\n" + helperMapClientsNameAndId.toString());
      System.out.println("--MapClientsNameAndId map after dissconnest--\n" + MapClientsNameAndId.toString());
      System.out.println("--MapAllUsers map after dissconnest--\n" + mapAllUsers.toString());
        ConnectionHandler<T> handler = connectionHandlers.remove(connectionId);
        if (handler != null) {
            System.out.println("i reach her handler!=null");
            try {
                handler.close(); // Close the connection handler
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Registers a new connection handler for a connection ID.
     *
     * @param connectionId the connection ID
     * @param handler the connection handler
     */
    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        connectionHandlers.put(connectionId, handler);
    }

    /**
     * Subscribes a connection to a channel.
     *
     * @param connectionId the connection ID
     * @param channel the channel name
     */

    /**
     * Unsubscribes a connection from a channel.
     *
     * @param connectionId the connection ID
     * @param channel the channel name
     */


   
    public void addSubscription (String channelName, String subscriptionId, int connectionId){
        if (topics.get(channelName)!=null){
            if (topics.get(channelName).get(connectionId)!=null){
                String lastSubId = topics.get(channelName).get(connectionId);
                helperTopics.get(connectionId).remove(lastSubId);
                helperTopics.get(connectionId).put(subscriptionId, channelName); 
                topics.get(channelName).remove(connectionId);
                topics.get(channelName).put(connectionId, subscriptionId);
            }
            else {
                topics.computeIfAbsent(channelName, key -> new ConcurrentHashMap<>()).put(connectionId, subscriptionId);   
                helperTopics.computeIfAbsent(connectionId, key -> new ConcurrentHashMap<>()).put(subscriptionId, channelName);
            }
        }
        else {
            topics.computeIfAbsent(channelName, key -> new ConcurrentHashMap<>()).put(connectionId, subscriptionId);   
            helperTopics.computeIfAbsent(connectionId, key -> new ConcurrentHashMap<>()).put(subscriptionId, channelName);
        }    
        
      
        //         helperTopics.computeIfAbsent(connectionId, key -> new ConcurrentHashMap<>()).put(subscriptionId, channelName);

        // }
        // else { //The channel name already exsits 
        //         topics.computeIfAbsent(channelName, key -> new ConcurrentHashMap<>()).put(connectionId, subscriptionId);      
        //         helperTopics.computeIfAbsent(connectionId, key -> new ConcurrentHashMap<>()).put(subscriptionId, channelName);
          
        // }
              System.out.println("--Topics map--\n" + topics.toString());
             System.out.println("--helperTopics map--\n" + helperTopics.toString());



        //מימוש שלא שומר את ה subId החדש
        // if (topics.get(channelName)==null){
        //     //new channel 
        //     topics.computeIfAbsent(channelName, key -> new ConcurrentHashMap<>()).put(connectionId, subscriptionId);      
        //         helperTopics.computeIfAbsent(connectionId, key -> new ConcurrentHashMap<>()).put(subscriptionId, channelName);

        // }
        // else { //The channel name already exsits 
        //     if (topics.get(channelName).get(connectionId)==null){
        //         topics.computeIfAbsent(channelName, key -> new ConcurrentHashMap<>()).put(connectionId, subscriptionId);      
        //         helperTopics.computeIfAbsent(connectionId, key -> new ConcurrentHashMap<>()).put(subscriptionId, channelName);
        //     }
        // }
        //       System.out.println("--Topics map--\n" + topics.toString());
        //      System.out.println("--helperTopics map--\n" + helperTopics.toString());

    //    // Check if the topic already exists
    //    if (topics.get(channelName)==null){
    //     topics.computeIfAbsent(channelName, key -> new ConcurrentHashMap<>());
    //    }
    //    if (topics.get(channelName).get(connectionId)==null){
    //    topics.get(channelName).put(connectionId, subscriptionId);
    //    }
    //     if (helperTopics.get(connectionId)==null){
    //         helperTopics.put(connectionId,new ConcurrentHashMap<>());
    //         helperTopics.get(connectionId).put(subscriptionId, channelName);
    //     }
    //    else {
    //     //The client take care is the client already subscribed
    //     topics.get(channelName).put(connectionId, subscriptionId);
    //     if (helperMapClientsNameAndId.get(connectionId)==null){
    //         helperTopics.put(connectionId,new ConcurrentHashMap<>());
    //         helperTopics.get(connectionId).put(subscriptionId, channelName);
    //     }
    //     else {
    //         helperTopics.get(connectionId).put(subscriptionId, channelName);
    //     }
    //    }
    //     System.out.println("--Topics map--\n" + topics.toString());
    //     System.out.println("--helperTopics map--\n" + helperTopics.toString());
    }

    public void removeSubscription(int coneectionId, String subscriptionId) {
        if (helperTopics.get(coneectionId)!=null){
            String channelName= helperTopics.get(coneectionId).get(subscriptionId);
            helperTopics.get(coneectionId).remove(subscriptionId);
            topics.get(channelName).remove(coneectionId);
        }
        System.out.println("--Topics map after unsubscribe--\n" + topics.toString());
        System.out.println("--helperTopics map after unsubscribe--\n" + helperTopics.toString());
    }
    
    public boolean isUserExists(String userName) {
        return mapAllUsers.containsKey(userName);
    }
    public boolean isClientExists(String userName){
        return MapClientsNameAndId.containsKey(userName); 
    }
    public String getPassward (String userName){
        return mapAllUsers.get(userName);
    }

    public void addClientToMapClientsNameAndId(String userName, int connectionId){
        this.MapClientsNameAndId.put(userName,connectionId);
        this.helperMapClientsNameAndId.put(connectionId,userName);
        System.out.println("--MapClientsNameAndId. map after login--\n" + MapClientsNameAndId.toString());
        System.out.println("--helperMapClientsNameAndId. map after login--\n" + helperMapClientsNameAndId.toString());
    }
    public void addClientToMapAllUsers(String userName, String passward){
        this.mapAllUsers.put(userName, passward);
        System.out.println("--addClientToMapAllUsers. map after login--\n" + mapAllUsers.toString());
    }

    //Getters: 
    public ConcurrentHashMap<Integer, ConnectionHandler<T>> getConnectionHandlers() {
        return connectionHandlers;
    }


    public ConcurrentHashMap<String, Integer> getMapClientsNameAndId() {
        return MapClientsNameAndId;
    }

    public ConcurrentHashMap<String, ConcurrentHashMap<Integer, String>> getTopics() {
        return topics;
    }

    public ConcurrentHashMap<Integer, ConcurrentHashMap<String,String>> getHeplperTopics() {
        return helperTopics;
    }

    public ConcurrentHashMap<String, String> getMapAllUsers() {
        return mapAllUsers;
    }

    public int getCounterMassageId() {
        return counterMassageId.get();
    }

    public int getAndIncreceMessageId(){
        return counterMassageId.incrementAndGet();   
    }

}
