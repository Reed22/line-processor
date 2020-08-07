use std::env; // to get arugments passed to the program
use std::thread;

/*
* Print the number of partitions and the size of each partition
* @param vs A vector of vectors
*/
fn print_partition_info(vs: &Vec<Vec<usize>>){
    println!("Number of partitions = {}", vs.len());
    for i in 0..vs.len(){
        println!("\tsize of partition {} = {}", i, vs[i].len());
    }
}

/*
* Create a vector with integers from 0 to num_elements -1
* @param num_elements How many integers to generate
* @return A vector with integers from 0 to (num_elements - 1)
*/
fn generate_data(num_elements: usize) -> Vec<usize>{
    let mut v : Vec<usize> = Vec::new();
    for i in 0..num_elements {
        v.push(i);
    }
    return v;
}

/*
* Partition the data in v into a 2 vectors
* @param num_partitions
* @param v 
* @return A vector that contains vectors of integers

*/
fn partition_data_in_two(v: &Vec<usize>) -> Vec<Vec<usize>>{
    let partition_size = v.len() / 2;
    let mut xs: Vec<Vec<usize>> = Vec::new();

    let mut x1 : Vec<usize> = Vec::new();
    for i in 0..partition_size{
        x1.push(v[i]);
    }
    xs.push(x1);

    let mut x2 : Vec<usize> = Vec::new();
    for i in partition_size..v.len(){
        x2.push(v[i]);
    }
    xs.push(x2);

    xs
}

/*
* Sum up the all the integers in the given vector
* @param v Vector of integers
* @return Sum of integers in v
* Note: this function has the same code as the reduce_data function.
*       But don't change the code of map_data or reduce_data.
*/
fn map_data(v: &Vec<usize>) -> usize{
    let mut sum = 0;
    for i in v{
        sum += i;
    }
    sum
}

/*
* Sum up the all the integers in the given vector
* @param v Vector of integers
* @return Sum of integers in v
*/
fn reduce_data(v: &Vec<usize>) -> usize{
    let mut sum = 0;
    for i in v{
        sum += i;
    }
    sum
}

/*
* A single threaded map-reduce program
*/
fn main() {

    // Use std::env to get arguments passed to the program
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        println!("ERROR: Usage {} num_partitions num_elements", args[0]);
        return;
    }
    let num_partitions : usize = args[1].parse().unwrap();
    let num_elements : usize = args[2].parse().unwrap();
    if num_elements < num_partitions{
        println!("ERROR: num_elements cannot be smaller than num_partitions");
        return;
    }

    // Generate data.
    let v = generate_data(num_elements);

    // PARTITION STEP: partition the data into 2 partitions
    let xs = partition_data_in_two(&v);

    // Print info about the partitions
    print_partition_info(&xs);

    let mut intermediate_sums : Vec<usize> = Vec::new();

    // MAP STEP: Process each partition

    // CHANGE CODE START: Don't change any code above this line

    // Change the following code to create 2 threads each of which must use map_data()
    // function to process one of the two partition

    //Make clones of xs[0] and xs[1]
    let clone1 = xs[0].clone();
    let clone2 = xs[1].clone();

    //Spawn 2 threads to handle map_data
    let t1 = thread::spawn(move|| map_data(&clone1) );
    let t2 = thread::spawn(move|| map_data(&clone2) );

    //Join threads and put result in t1_res
    let t1_res = t1.join().unwrap();
    let t2_res = t2.join().unwrap();

    intermediate_sums.push(t1_res);
    intermediate_sums.push(t2_res);

    // CHANGE CODE END: Don't change any code below this line until the next CHANGE CODE comment

    // Print the vector with the intermediate sums
    println!("Intermediate sums = {:?}", intermediate_sums);

    // REDUCE STEP: Process the intermediate result to produce the final result
    let sum = reduce_data(&intermediate_sums);
    println!("Sum = {}", sum);

    // CHANGE CODE: Add code that does the following:
    // 1. Calls partition_data to partition the data into equal partitions
    let new_vec = partition_data(num_partitions, &v);

    // 2. Calls print_partiion_info to print info on the partitions that have been created
    print_partition_info(&new_vec);

    // 3. Creates one thread per partition and uses each thread to process one partition
    let mut thread_handles = Vec::new();
    for i in 0..num_partitions{
        let new_clone = new_vec[i].clone();
        thread_handles.push(thread::spawn(move|| map_data(&new_clone)))
    }

    // 4. Collects the intermediate sums from all the threads
    let mut results_vec = Vec::new();
    for handle in thread_handles{
        let result = handle.join().unwrap();
        results_vec.push(result);
    }
    // 5. Prints information about the intermediate sums
    println!("Intermediate sums = {:?}", results_vec);

    // 5. Calls reduce_data to process the intermediate sums
    let sum = reduce_data(&results_vec);

    // 6. Prints the final sum computed by reduce_data
    println!("Sum = {}", sum);

}

/*
* CHANGE CODE: code this function
* Note: Don't change the signature of this function
*
* Partitions the data into a number of partitions such that
*   - the returned partitions contain all elements that are in the input vector
*   - all partitions (except possibly the the last one) have equal number 
*      of elements. The last partition may have either the same number of
*      elements or fewer elements than the other partitions.
*
* @param num_partitions The number of partitions to create
* @param v The data to be partitioned
* @return A vector that contains vectors of integers
* 
*/
fn partition_data(num_partitions: usize, v: &Vec<usize>) -> Vec<Vec<usize>>{
    
    //partition size will be length of data vector divided by number of partitions 
    let partition_size = v.len() / num_partitions;
    let mut xv: Vec<Vec<usize>> = Vec::new();
    
    let mut v_index = 0;
    
    //Create a vector for each partition 
    for _j in 0..num_partitions{
        let mut x1 : Vec<usize> = Vec::new();
        //Fill each partition vector
        for _i in 0..partition_size{
            x1.push(v[v_index]);
            v_index = v_index + 1;
        }
        //Push partition vector to combined vector
        xv.push(x1);
    }

    xv
}
