import ns.internet
import ns.core

class thomasQLRNHelper(ns.internet.Ipv4RoutingHelper):
    def __init__(self):
        self.m_objectFactory = ns.core.ObjectFactory("ns3::tqlrn::RoutingProtocol") #or what ?
    # /**
    #  * \returns pointer to clone of this OlsrHelper
    #  *
    #  * \internal
    #  * This method is mainly for internal use by the other helpers;
    #  * clients are expected to free the dynamic memory allocated by this method
    #  */
    # AodvHelper* Copy (void) const;
    def Copy(self) :
        pass

    # /**
    #  * \param node the node on which the routing protocol will run
    #  * \returns a newly-created routing protocol
    #  *
    #  * This method will be called by ns3::InternetStackHelper::Install
    #  *
    #  * \todo support installing AODV on the subset of all available IP interfaces
    #  */
    # virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
    def Create(self, node):
        pass
    # /**
    #  * \param name the name of the attribute to set
    #  * \param value the value of the attribute to set.
    #  *
    #  * This method controls the attributes of ns3::aodv::RoutingProtocol
    #  */
    # void Set (std::string name, const AttributeValue &value);
    def Set(name,value):
        pass
    # /**
    #  * Assign a fixed random variable stream number to the random variables
    #  * used by this model.  Return the number of streams (possibly zero) that
    #  * have been assigned.  The Install() method of the InternetStackHelper
    #  * should have previously been called by the user.
    #  *
    #  * \param stream first stream index to use
    #  * \param c NodeContainer of the set of nodes for which AODV
    #  *          should be modified to use a fixed stream
    #  * \return the number of stream indices assigned by this helper
    #  */
    # int64_t AssignStreams (NodeContainer c, int64_t stream);
    def AssignStreams (nodes, stream):
        pass

    # private:
    # /** the factory to create AODV routing object */
    # ObjectFactory m_agentFactory;
    #  que .?
