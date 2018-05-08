void qs(int* arr, int s, int e)
{
	if(s >= e-1) return;
	int key = arr[s];
	int i = s, j = e-1;
	while(i < j)
	{
		while(i < j && arr[j] >= key)
			j--;
		arr[i] = arr[j];
		while(i < j && arr[i] <= key)
			i++;
		arr[j] = arr[i];
	}
	arr[i] = key;
	qs(arr, s, i);
	qs(arr, i+1, e);
	return;
}

int arr[40000];

int main()
{
	for(int i = 0; i < 40000; i++)
		arr[i] = 40000 - (6133 * i) % 40000;//6133 is the 800th prime number
	qs(arr, 0, 40000);
}
