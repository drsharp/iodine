
module Iodine
	module Http

		# This class is a good demonstration for creating a Websocket handler with the Iodine API.
		#
		# Iodine is Object Oriented and for this reason the Websocket handler is expected to
		# retain the information it needs - either through initialization, or through the `on_open(protocol)` callback.
		class WebsocketHandler
			# The original Http request
			attr_reader :request 
			# The Http response, also allowing for websocket data
			attr_reader :response
			# this is called while still communicating over Http (during the upgrade process).
			def initialize request, response
				@request = request
				@response = response
			end
			# initialize the protocol data once the connection had opened.
			def on_open
			end
			# Accept data using this callback - this is a required callback.
			def on_message data
			end
			# Accept unicasts or broadcasts using this callback.
			def on_broadcast data
			end
			# cleanup, if needed, using this callback.
			def on_close
			end
			# extra cleanup, if needed, when server is shutting down while the websocket is connected.
			#
			# You can use on_close unless some "going away" cleanup is required.
			def on_shutdown
			end

			# This method allows the class itself to act as the Websocket handler, usable with:
			#
			#           # Iodine::Http::WebsocketHandler's default implementation does nothing.
			#           Iodine::Http.on_websocket Iodine::Http::WebsocketHandler
			def self.call request, response
				self.new request, response
			end
			
			protected

			### some helper methods

			# Write data to the client, using Websockets encoded frames.
			def write data
				# We leverage the fact that the Http response can be used to send Websocket data.
				#
				# you can also use Websocket#send_data or it's alias Websocket#<< for example:
				#
				#        # @request[:io] contains the Websockets Protocol instance
				#        @request[:io] << data
				#
				# do NOT use Websocket#write (which writes the data directly, bypassing the protocol).
				@response << data
			end

			# Send messages directly to a specific Websocket.
			#
			# This implementation is limited to a single process on a single server.
			# Consider using Redis for a scalable implementation.
			def unicast id, data
				::Iodine::Http::Websockets.unicast id, data
			end
			# Broadcast to all Websockets, except self.
			#
			# This implementation is limited to a single process on a single server.
			# Consider using Redis for a scalable implementation.
			def broadcast data
				::Iodine::Http::Websockets.broadcast data, self
			end
			# Closes the connection
			def close
				# @request[:io] contains the Websockets Protocol instance
				@request[:io].go_away
			end
		end
	end
end


