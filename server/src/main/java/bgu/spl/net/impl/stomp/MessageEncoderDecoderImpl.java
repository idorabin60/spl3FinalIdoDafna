package bgu.spl.net.impl.stomp;

// package bgu.spl.net.api;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import bgu.spl.net.api.MessageEncoderDecoder;

public class MessageEncoderDecoderImpl implements MessageEncoderDecoder<Frame> {

    private final List<Byte> bytes = new ArrayList<>(); // Temporary buffer for incoming bytes

    @Override
    public Frame decodeNextByte(byte nextByte) {
        if (nextByte == '\0') { // End-of-frame delimiter detected
            byte[] messageBytes = new byte[bytes.size()];
            for (int i = 0; i < bytes.size(); i++) {
                messageBytes[i] = bytes.get(i);
            }

            String message = new String(messageBytes, StandardCharsets.UTF_8);
            bytes.clear(); // Clear the buffer for the next message
            return Frame.deserialize(message); // Use Frame's deserialize method
        } else {
            bytes.add(nextByte);
            return null; // Frame not yet complete
        }
    }

    @Override
    public byte[] encode(Frame message) {
        // Use Frame's serialize method to convert it to a string and append '\0'
        String serializedMessage = message.serialize();
        return serializedMessage.getBytes(StandardCharsets.UTF_8);
    }
}
