package org.ccnx.ccn.test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Map.Entry;

import org.ccnx.ccn.CCNInterestListener;
import org.ccnx.ccn.CCNHandle;
import org.ccnx.ccn.config.ConfigurationException;
import org.ccnx.ccn.impl.support.DataUtils;
import org.ccnx.ccn.impl.support.Log;
import org.ccnx.ccn.protocol.ContentName;
import org.ccnx.ccn.protocol.ContentObject;
import org.ccnx.ccn.protocol.ExcludeComponent;
import org.ccnx.ccn.protocol.Exclude;
import org.ccnx.ccn.protocol.Interest;
import org.ccnx.ccn.protocol.MalformedContentNameStringException;


/**
 *  A class to help write tests without a repo. Pulls things
 *  through ccnd, under a set of namespaces.
 *  Cheesy -- uses excludes to not get the same content back.
 *  
 *  TODO FIX -- not entirely correct. Gets more duplicates than it needs to.
 *  
 *  See CCNVersionedInputStream for related stream-based flossing code.
 *  The "floss" term refers to "mental floss" -- think a picture of
 *  someone running dental floss in and out through their ears.
 * @author smetters
 *
 */
public class Flosser implements CCNInterestListener {
	
	CCNHandle _library;
	Map<ContentName, Interest> _interests = new HashMap<ContentName, Interest>();
	Map<ContentName, Set<ContentName>> _subInterests = new HashMap<ContentName, Set<ContentName>>();
	HashSet<ContentObject> _processedObjects = new HashSet<ContentObject>();
	
	/**
	 * Constructors that called handleNamespace() now throwing NullPointerException as this doesn't exist yet.
	 * @throws ConfigurationException
	 * @throws IOException
	 */
	public Flosser() throws ConfigurationException, IOException {
		_library = CCNHandle.open();
	}
	
	public Flosser(ContentName namespace) throws ConfigurationException, IOException {
		this();
		handleNamespace(namespace);
	}
	
	public void handleNamespace(String namespace) throws MalformedContentNameStringException, IOException {
		handleNamespace(ContentName.fromNative(namespace));
	}
	
	public void stopMonitoringNamespace(String namespace) throws MalformedContentNameStringException {
		stopMonitoringNamespace(ContentName.fromNative(namespace));
	}
	
	public void stopMonitoringNamespace(ContentName namespace) {
		synchronized(_interests) {
			if (!_interests.containsKey(namespace)) {
				return;
			}
			Set<ContentName> subInterests = _subInterests.get(namespace);
			if (null != subInterests) {
				Iterator<ContentName> it = subInterests.iterator();
				while (it.hasNext()) {
					ContentName subNamespace = it.next();
					removeInterest(subNamespace);
					it.remove();
				}
			}
			// Remove the top-level interest.
			removeInterest(namespace);
			_subInterests.remove(namespace);
			Log.info("FLOSSER: no longer monitoring namespace: " + namespace);
		}
	}
	
	protected void removeInterest(ContentName namespace) {
		synchronized(_interests) {
			if (!_interests.containsKey(namespace)) {
				return;
			}
			Interest interest = _interests.get(namespace);
			_library.cancelInterest(interest, this);
			_interests.remove(namespace);
			Log.fine("Cancelled interest in " + namespace);
		}
	}
	
	/**
	 * Handle a top-level namespace.
	 * @param namespace
	 * @throws IOException
	 */
	public void handleNamespace(ContentName namespace) throws IOException {
		synchronized(_interests) {
			if (_interests.containsKey(namespace)) {
				Log.fine("Already handling namespace: " + namespace);
				return;
			}
			Log.info("Flosser: handling namespace: " + namespace);
			Interest namespaceInterest = new Interest(namespace);
			_interests.put(namespace, namespaceInterest);
			_library.expressInterest(namespaceInterest, this);
			Set<ContentName> subNamespaces = _subInterests.get(namespace);
			if (null == subNamespaces) {
				subNamespaces = new HashSet<ContentName>();
				_subInterests.put(namespace, subNamespaces);
				Log.info("FLOSSER: setup parent namespace: " + namespace);
			}			
		}
	}
	
	public void handleNamespace(ContentName namespace, ContentName parent) throws IOException {
		synchronized(_interests) {
			if (_interests.containsKey(namespace)) {
				Log.fine("Already handling child namespace: " + namespace);
				return;
			}
			Log.info("FLOSSER: handling child namespace: " + namespace + " expected parent: " + parent);
			Interest namespaceInterest = new Interest(namespace);
			_interests.put(namespace, namespaceInterest);
			_library.expressInterest(namespaceInterest, this);
			
			// Now we need to find a parent in the subInterest map, and reflect this namespace underneath it.
			ContentName parentNamespace = parent;
			Set<ContentName> subNamespace = _subInterests.get(parentNamespace);
			while ((subNamespace == null) && (!parentNamespace.equals(ContentName.ROOT))) {
				parentNamespace = parentNamespace.parent();
				subNamespace = _subInterests.get(parentNamespace);
				Log.info("FLOSSER: initial parent not found in map, looked up " + parentNamespace + " found in map? " + ((null == subNamespace) ? "no" : "yes"));
			}
			if (null != subNamespace) {
				Log.info("FLOSSER: Adding subnamespace: " + namespace + " to ancestor " + parentNamespace);
				subNamespace.add(namespace);
			} else {
				Log.info("FLOSSER: Cannot find ancestor namespace for " + namespace);
				for (ContentName n : _subInterests.keySet()) {
					Log.info("FLOSSER: 		available ancestor: " + n);
				}
			}
		}
	}

	public Interest handleContent(ArrayList<ContentObject> results,
								  Interest interest) {
		Log.finest("Interests registered: " + _interests.size() + " content objects returned: "+results.size());
		// Parameterized behavior that subclasses can override.
		ContentName interestName = null;
		for (ContentObject result : results) {
			if (_processedObjects.contains(result)) {
				Log.fine("Got repeated content for interest: " + interest + " content: " + result.name());
				continue;
			}
			Log.finest("Got new content for interest " + interest + " content name: " + result.name());
			processContent(result);
			// update the interest. follow process used by ccnslurp.
            // exclude the next component of this object, and set up a
            // separate interest to explore its children.
			// first, remove the interest from our list as we aren't going to
			// reexpress it in exactly the same way
			synchronized(_interests) {
				for (Entry<ContentName, Interest> entry : _interests.entrySet()) {
					if (entry.getValue().equals(interest)) {
						interestName = entry.getKey();
						_interests.remove(interestName);
						break;
					}
				}
			}

            int prefixCount = interest.name().count();
            // DKS TODO should the count above be count()-1 and this just prefixCount?
            if (prefixCount == result.name().count()) {
            	if (null == interest.exclude()) {
              		ArrayList<Exclude.Element> excludes = new ArrayList<Exclude.Element>();
               		excludes.add(new ExcludeComponent(result.contentDigest()));
            		interest.exclude(new Exclude(excludes));
            		Log.finest("Creating new exclude filter for interest " + interest.name());
            	} else {
            		if (interest.exclude().match(result.contentDigest())) {
            			Log.fine("We should have already excluded content digest: " + DataUtils.printBytes(result.contentDigest()));
            		} else {
            			// Has to be in order...
            			Log.finest("Adding child component to exclude.");
            			interest.exclude().add(new byte [][] { result.contentDigest() });
            		}
            	}
            	Log.finer("Excluding content digest: " + DataUtils.printBytes(result.contentDigest()) + " onto interest " + interest.name() + " total excluded: " + interest.exclude().size());
            } else {
               	if (null == interest.exclude()) {
               		ArrayList<Exclude.Element> excludes = new ArrayList<Exclude.Element>();
               		excludes.add(new ExcludeComponent(result.name().component(prefixCount)));
            		interest.exclude(new Exclude(excludes));
            		Log.finest("Creating new exclude filter for interest " + interest.name());
               	} else {
                    if (interest.exclude().match(result.name().component(prefixCount))) {
            			Log.fine("We should have already excluded child component: " + ContentName.componentPrintURI(result.name().component(prefixCount)));                   	
                    } else {
                    	// Has to be in order...
                    	Log.finest("Adding child component to exclude.");
            			interest.exclude().add(
            					new byte [][] { result.name().component(prefixCount) });
                    }
            	}
               	Log.finer("Excluding child " + ContentName.componentPrintURI(result.name().component(prefixCount)) + " total excluded: " + interest.exclude().size());
                // DKS TODO might need to split to matchedComponents like ccnslurp
                ContentName newNamespace = null;
                try {
                	if (interest.name().count() == result.name().count()) {
                		newNamespace = new ContentName(interest.name(), result.contentDigest());
                	} else {
                		newNamespace = new ContentName(interest.name(), 
                			result.name().component(interest.name().count()));
                	}
                	Log.info("Adding new namespace: " + newNamespace);
                	handleNamespace(newNamespace, interest.name());
                } catch (IOException ioex) {
                	Log.warning("IOException picking up namespace: " + newNamespace);
                }
            }
		}
		if (null != interest)
			synchronized(_interests) {
				_interests.put(interest.name(), interest);
			}
		return interest;
	}
	
	public void stop() {
		Log.info("Stop flossing.");
		synchronized (_interests) {
			for (Interest interest : _interests.values()) {
				Log.info("Cancelling pending interest: " + interest);
				_library.cancelInterest(interest, this);
			}
		}
	}
	
	public void logNamespaces() {
		
		ContentName [] namespaces = getNamespaces().toArray(new ContentName[getNamespaces().size()]);
		for (ContentName name : namespaces) {
			Log.info("Flosser: monitoring namespace: " + name);
		}
	}
	public Set<ContentName> getNamespaces() {
		synchronized (_interests) {
			return _interests.keySet();
		}
	}

	/**
	 * Override in subclasses that want to do something more interesting than log.
	 * @param result
	 */
	protected void processContent(ContentObject result) {
		Log.info("Flosser got: " + result.name() + " Digest: " + 
				DataUtils.printBytes(result.contentDigest()));
	}
	
}