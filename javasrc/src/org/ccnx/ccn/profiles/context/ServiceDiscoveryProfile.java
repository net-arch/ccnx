/**
 * Part of the CCNx Java Library.
 *
 * Copyright (C) 2010 Palo Alto Research Center, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation. 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details. You should have received
 * a copy of the GNU Lesser General Public License along with this library;
 * if not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA 02110-1301 USA.
 */
package org.ccnx.ccn.profiles.context;

import java.io.IOException;
import java.util.ArrayList;

import org.ccnx.ccn.CCNHandle;
import org.ccnx.ccn.KeyManager;
import org.ccnx.ccn.profiles.CCNProfile;
import org.ccnx.ccn.profiles.CommandMarker;
import org.ccnx.ccn.profiles.security.KeyProfile;
import org.ccnx.ccn.protocol.ContentName;
import org.ccnx.ccn.protocol.ContentObject;
import org.ccnx.ccn.protocol.Interest;
import org.ccnx.ccn.protocol.PublisherPublicKeyDigest;

/**
 * The ServiceDiscovery protocol aids in finding data about local (same-machine)
 * and nearby (network neighborhood) services. We start by retrieving service keys, using:
 *  /localhost/%C1.M.SVC/<servicename>/KEY/
 *  and getting as an answer an encoded, self-signed key published under the 
 *  KeyProfile.keyName() of that service's key under that prefix. 
 */
public class ServiceDiscoveryProfile implements CCNProfile {

	public static final CommandMarker SERVICE_NAME_COMPONENT_MARKER = 
		CommandMarker.commandMarker(CommandMarker.MARKER_NAMESPACE, "SVC");
		
	public static ContentName localServiceName(String service) {
		return new ContentName(ContextualNamesProfile.LOCALHOST, SERVICE_NAME_COMPONENT_MARKER.getBytes(), 
				ContentName.componentParseNative(service));
	}
	
	/**
	 * We want to get a list of local implementors of this service and their keys; until the
	 * timeout. We use excludes to get all of them within the timeout. We get whole keys because
	 * they're usually a single object, so there is no reason not to load them. Rather than decoding
	 * them all, though, we just hand back the CO to the caller to decide what to do.
	 * 
	 * Start by taking a timeout to use as the inter-response interval; if we haven't heard anything
	 * in that long, we stop. 
	 * @param service
	 * @param serviceKey
	 * @param keyManager
	 * @throws IOException 
	 */
	public ArrayList<ContentObject> getLocalServiceKeys(String service, long timeout, CCNHandle handle) throws IOException {
		
		ContentName serviceKeyName = new ContentName(localServiceName(service), KeyProfile.KEY_NAME_COMPONENT);
		
		// Construct an interest in anything below this that has the right form -- this prefix, a component
		// for the key id, and then a version and segments. Might be more expensive to apply the filters than
		// to throw things away and go around again...
		
		Interest theInterest = new Interest();
		
		ArrayList<ContentObject> results = null;
		ContentObject theResult = null;
		int keyidComponent = serviceKeyName.count();
		
		ArrayList<byte[]> excludeList = new ArrayList<byte[]>();
		
		do {
			
			if (excludeList.size() > 0) {
				//we have explicit excludes, add them to this interest
				byte [][] e = new byte[excludeList.size()][];
				excludeList.toArray(e);
				theInterest.exclude().add(e);
			}
			theResult = handle.get(theInterest, timeout);
			
			// Verify theResult
			
			
			// Check to see if theResult matches criteria
			
			if (null != theResult) {
				if (null == results) {
					results = new ArrayList<ContentObject>();
				}
				results.add(theResult);
				excludeList.add(theResult.name().component(keyidComponent));
			}

		} while (null != theResult);
		
		return results;
	}
	
	public static void publishLocalServiceKey(String service, PublisherPublicKeyDigest serviceKey, KeyManager keyManager) {
		
	}

}