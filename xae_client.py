#!/usr/bin/env python3
"""
XAE OS Network Client
Connects to XAE OS with encryption and authentication
"""

import socket
import sys
import select
import termios
import tty

ENCRYPTION_KEY = 0x42

def encrypt_data(data):
    """XOR encrypt data"""
    return bytes([b ^ ENCRYPTION_KEY for b in data])

def decrypt_data(data):
    """XOR decrypt data (symmetric)"""
    return encrypt_data(data)

class XAEClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        
    def connect(self):
        """Connect to XAE OS"""
        print(f"Connecting to XAE OS at {self.host}:{self.port}...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        print("Connected!")
        
    def login(self, username, password):
        """Authenticate with XAE OS"""
        auth_str = f"{username}:{password}\n"
        encrypted = encrypt_data(auth_str.encode())
        self.sock.send(encrypted)
        
        # Wait for response
        response = self.sock.recv(1024)
        decrypted = decrypt_data(response).decode('utf-8', errors='ignore')
        print(decrypted, end='')
        
        if "Welcome" in decrypted:
            return True
        return False
        
    def send_command(self, command):
        """Send encrypted command to XAE OS"""
        cmd = command + "\n"
        encrypted = encrypt_data(cmd.encode())
        self.sock.send(encrypted)
        
    def receive_response(self):
        """Receive and decrypt response"""
        try:
            data = self.sock.recv(4096)
            if data:
                decrypted = decrypt_data(data).decode('utf-8', errors='ignore')
                return decrypted
        except:
            return None
        return None
        
    def interactive_session(self):
        """Interactive terminal session"""
        # Save terminal settings
        old_settings = termios.tcgetattr(sys.stdin)
        
        try:
            # Set terminal to raw mode
            tty.setraw(sys.stdin.fileno())
            
            while True:
                # Check for input from both socket and stdin
                readable, _, _ = select.select([self.sock, sys.stdin], [], [], 0.1)
                
                for source in readable:
                    if source is self.sock:
                        # Data from server
                        response = self.receive_response()
                        if response:
                            sys.stdout.write(response)
                            sys.stdout.flush()
                        else:
                            print("\n\nConnection closed by server")
                            return
                            
                    elif source is sys.stdin:
                        # Data from user
                        char = sys.stdin.read(1)
                        
                        if char == '\x03':  # Ctrl+C
                            print("\n\nDisconnecting...")
                            return
                            
                        # Send character
                        encrypted = encrypt_data(char.encode())
                        self.sock.send(encrypted)
                        
        finally:
            # Restore terminal settings
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
            
    def close(self):
        """Close connection"""
        if self.sock:
            self.sock.close()

def main():
    if len(sys.argv) < 3:
        print("Usage: python xae_client.py <host> <port> [username] [password]")
        print("Example: python xae_client.py 10.0.0.2 23 admin admin123")
        sys.exit(1)
        
    host = sys.argv[1]
    port = int(sys.argv[2])
    username = sys.argv[3] if len(sys.argv) > 3 else input("Username: ")
    password = sys.argv[4] if len(sys.argv) > 4 else input("Password: ")
    
    client = XAEClient(host, port)
    
    try:
        client.connect()
        
        if client.login(username, password):
            print("\n=== Connected to XAE OS ===")
            print("Press Ctrl+C to disconnect\n")
            client.interactive_session()
        else:
            print("Authentication failed!")
            
    except KeyboardInterrupt:
        print("\n\nDisconnected")
    except Exception as e:
        print(f"\nError: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    main()
