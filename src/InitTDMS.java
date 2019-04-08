import java.io.File;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import com.google.common.collect.ImmutableMap;
import alluxio.AlluxioURI;


import alluxio.client.file.FileSystem;
import alluxio.client.file.URIStatus;
import alluxio.exception.FileAlreadyExistsException;
import alluxio.exception.FileDoesNotExistException;
import alluxio.*;
import alluxio.exception.AlluxioException;
import alluxio.Configuration;
import alluxio.client.file.BaseFileSystem;
import alluxio.client.file.options.CreateFileOptions;
import alluxio.client.file.policy.FileWriteLocationPolicy;
import alluxio.client.file.policy.LocalFirstPolicy;
import alluxio.client.file.policy.RoundRobinPolicy;
import alluxio.client.file.policy.MostAvailableFirstPolicy;
import alluxio.client.file.policy.SpecificHostPolicy;
import alluxio.wire.FileBlockInfo;
import alluxio.wire.BlockInfo;
import alluxio.wire.BlockLocation;
import alluxio.wire.WorkerNetAddress;
import alluxio.worker.block.BlockMetadataManager;
import alluxio.worker.block.meta.StorageTier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import alluxio.util.ConfigurationUtils;
import java.util.Properties;
import java.util.ArrayList;
import java.util.List;
import java.util.Iterator;

public class InitTDMS{

private static String myinputfile=null;
private static String mymount_point="/home/condor/alluxio-data";
private static int sNum = 0;
private static int sI = 0;
private static StorageTier sTier0;
private static StorageTier sTier1;

public InitTDMS(){}


public static FileSystem gethost(){

	String confile = "/home/condor/alluxio/conf/alluxio-site.properties";
        Properties mypro = ConfigurationUtils.loadPropertiesFromFile(confile);
        Configuration.merge(mypro);

	FileSystem fs = FileSystem.Factory.get();

    return fs;
} 

public static FileWriteLocationPolicy getLocationPolicy(String mypolicy, String targethost ){
	FileWriteLocationPolicy localgetLocationPolicy = new LocalFirstPolicy();
	switch(mypolicy){
	    case "PIPELINE":{
		break;
	    }
	    case "SCATTER":{
		//localgetLocationPolicy = new LocalFirstPolicy();
		localgetLocationPolicy = new RoundRobinPolicy();
		break;
	    }
	    case "GATHER":{
	        if(targethost != null){
		    localgetLocationPolicy = new SpecificHostPolicy(targethost);
		    break;
		}else{
		    System.out.printf("GATHER Pattern without target host\n");
		    localgetLocationPolicy = new MostAvailableFirstPolicy();
		    break; 
		}	    
	    }
	    case "MULTICAST":{
		localgetLocationPolicy = new RoundRobinPolicy();
		break;
	    } 
	    case "REDUCE":{
		localgetLocationPolicy = new RoundRobinPolicy();
                break;
	    } default : {
		 System.out.printf("Unknown Pattern error\n");
	    }
	}
	return localgetLocationPolicy;
}

public static int getLoadBalanceStrategy(String myloadbalance){
	int mytier = 0;
	switch(myloadbalance){
	    case "MaxFree":{
		System.out.printf("MaxFree LoadBalanceStrategy\n");
	    	BlockMetadataManager mMetaManager = BlockMetadataManager.createBlockMetadataManager();
		sTier0 = mMetaManager.getTier("MEM");
		sTier1 = mMetaManager.getTier("HDD");
		long freeTier0 = sTier0.getAvailableBytes();
    		long avaTier0 = sTier0.getCapacityBytes();
    		long freeTier1 = sTier1.getAvailableBytes();
	        long avaTier1 = sTier1.getCapacityBytes();
	        double f0 = ((double) freeTier0) / ((double) avaTier0);
	        double f1 = ((double) freeTier1) / ((double) avaTier1);
    		System.out.printf("Avaliable space of Tier 0 is %f \n",f0);
		System.out.printf("Avaliable space of Tier 1 is %f \n",f1);
		if (f0 < f1) {
      		    mytier=2;
    		} else {
      		    mytier=0;
    		}
		break;
	    }
	    case "RoundRobin":{
		System.out.printf("RoundRobin LoadBalanceStrategy\n");
		sNum++;
    		if (sNum == 1000) {
      		    sNum = 0;
    		}
	        sI = (sNum - 1) % 2;
    		mytier=sI;
		break;
	    }
	    case "Random":{
		System.out.printf("Random LoadBalanceStrategy\n");
		sI = (int) (Math.random() * 2);
    		mytier=sI;
		break;
            }default :{
		 System.out.printf("Unknown LoadBalanceStrategy error\n");
	    }
	}
	return mytier;
}

}













