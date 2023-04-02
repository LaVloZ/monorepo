import { useState } from 'react';
import './App.css';
import salade from './salade.webp'

function App() {

  const [state, setState] = useState({ 
    name: "Salade césarienne",
    picture: salade,
    price: '5,00 €',
  });

  return (
    <div className="App">
      <header className="App-header">
        <div>
          <div>
            {state.name}
          </div>
          <div>
            <img src={state.picture} alt={state.name} />
          </div>
          <div>
            {state.price}
          </div>
        </div>
      </header>
    </div>
  );
}

export default App;
