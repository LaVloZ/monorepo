import { useEffect, useState } from 'react';
import './App.css';
import salade from './salade.webp'

function App() {

  const [dish, setDish] = useState({});

  useEffect(() => {
    setDish({
      name: 'Salade césarienne',
      picture: salade,
      price: '5,00 €',
    })
  }, [])

  return (
    <div className="App">
      <header className="App-header">
        <div>
          <div>
            {dish.name}
          </div>
          <div>
            <img src={dish.picture} alt={dish.name} />
          </div>
          <div>
            {dish.price}
          </div>
        </div>
      </header>
    </div>
  );
}

export default App;
